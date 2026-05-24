// Copyright © 2022 Collabora, Ltd.
// SPDX-License-Identifier: MIT

use crate::ir::*;
use crate::legalize::{
    src_is_reg, src_is_upred_reg, swap_srcs_if_not_reg, LegalizeBuildHelpers,
    LegalizeBuilder,
};
use crate::sm70::ShaderModel70;
use bitview::*;

use rustc_hash::FxHashMap;
use std::mem;
use std::ops::Range;

/// A per-op trait that implements Volta+ opcode semantics
trait SM70Op {
    fn legalize(&mut self, b: &mut LegalizeBuilder);
    fn encode(&self, e: &mut SM70Encoder<'_>);
}

struct SM70Encoder<'a> {
    sm: u8,
    ip: usize,
    labels: &'a FxHashMap<Label, usize>,
    inst: [u32; 4],
}

impl BitViewable for SM70Encoder<'_> {
    fn bits(&self) -> usize {
        BitView::new(&self.inst).bits()
    }

    fn get_bit_range_u64(&self, range: Range<usize>) -> u64 {
        BitView::new(&self.inst).get_bit_range_u64(range)
    }
}

impl BitMutViewable for SM70Encoder<'_> {
    fn set_bit_range_u64(&mut self, range: Range<usize>, val: u64) {
        BitMutView::new(&mut self.inst).set_bit_range_u64(range, val);
    }
}

impl SM70Encoder<'_> {
    /// Maximum encodable UGPR
    ///
    /// This may be different from the actual maximum supported by hardware.
    fn ugpr_max(&self) -> u32 {
        if self.sm >= 100 {
            255
        } else {
            63
        }
    }

    fn zero_reg(&self, file: RegFile) -> RegRef {
        let nr = match file {
            RegFile::GPR => 255,
            RegFile::UGPR => self.ugpr_max(),
            _ => panic!("Not a GPR"),
        };
        RegRef::new(file, nr, 1)
    }

    fn true_reg(&self, file: RegFile) -> RegRef {
        RegRef::new(file, 7, 1)
    }

    fn set_opcode(&mut self, opcode: u16) {
        self.set_field(0..12, opcode);
    }

    fn set_reg(&mut self, range: Range<usize>, reg: RegRef) {
        assert!(range.len() == 8);
        assert!(reg.file() == RegFile::GPR);
        self.set_field(range, reg.base_idx());
    }

    fn set_ureg(&mut self, start: usize, reg: RegRef) {
        // Before SM100 UGPRs encode in 6 bits and there are instructions using
        // the two bits that follow.
        let ureg_size = if self.sm >= 100 { 8 } else { 6 };

        assert!(self.sm >= 73);
        assert!(reg.file() == RegFile::UGPR);
        assert!(reg.base_idx() <= self.ugpr_max());
        let range = Range {
            start: start,
            end: start + ureg_size,
        };

        self.set_field(range, reg.base_idx());
    }

    fn set_pred_reg(&mut self, range: Range<usize>, reg: RegRef) {
        assert!(range.len() == 3);
        assert!(reg.base_idx() <= 7);
        assert!(reg.comps() == 1);
        self.set_field(range, reg.base_idx());
    }

    fn set_reg_src(&mut self, range: Range<usize>, src: &Src) {
        assert!(src.is_unmodified());
        match src.src_ref {
            SrcRef::Zero => self.set_reg(range, self.zero_reg(RegFile::GPR)),
            SrcRef::Reg(reg) => self.set_reg(range, reg),
            _ => panic!("Not a register"),
        }
    }

    fn set_reg_addr(
        &mut self,
        range: Range<usize>,
        src: &Src,
        size_bit: usize,
    ) {
        assert!(src.is_unmodified());
        match src.src_ref {
            SrcRef::Zero => {
                self.set_reg(range, self.zero_reg(RegFile::GPR));
                // We always treat a zero GPR as 32 bits, so the UGPR source
                // can be 32 bits.
                self.set_bit(size_bit, false);
            }
            SrcRef::Reg(reg) => {
                self.set_reg(range, reg);
                assert!(reg.comps() <= 2);
                self.set_bit(size_bit, reg.comps() == 2);
            }
            _ => panic!("Not a register"),
        }
    }

    fn set_ureg_src(&mut self, start: usize, src: &Src) {
        assert!(src.src_mod.is_none());
        match src.src_ref {
            SrcRef::Zero => self.set_ureg(start, self.zero_reg(RegFile::UGPR)),
            SrcRef::Reg(reg) => self.set_ureg(start, reg),
            _ => panic!("Not a register"),
        }
    }

    fn set_ureg_addr(&mut self, start: usize, src: &Src, size_bit: usize) {
        assert!(src.src_mod.is_none());
        match src.src_ref {
            SrcRef::Zero => {
                self.set_ureg(start, self.zero_reg(RegFile::UGPR));
                // We always treat a zero UGPR as 64 bits, so the GPR source
                // can be 64 bit.
                self.set_bit(size_bit, true);
            }
            SrcRef::Reg(reg) => {
                self.set_ureg(start, reg);
                assert!(reg.comps() <= 2);
                self.set_bit(size_bit, reg.comps() == 2);
            }
            _ => panic!("Not a register"),
        }
    }

    fn set_pred_dst(&mut self, range: Range<usize>, dst: &Dst) {
        match dst {
            Dst::None => self.set_pred_reg(range, self.true_reg(RegFile::Pred)),
            Dst::Reg(reg) => self.set_pred_reg(range, *reg),
            _ => panic!("Not a register"),
        }
    }

    fn set_pred_src_file(
        &mut self,
        range: Range<usize>,
        not_bit: usize,
        src: &Src,
        file: RegFile,
    ) {
        let (not, reg) = match src.src_ref {
            SrcRef::True => (false, self.true_reg(file)),
            SrcRef::False => (true, self.true_reg(file)),
            SrcRef::Reg(reg) => {
                assert!(reg.file() == file);
                (false, reg)
            }
            _ => panic!("Not a register"),
        };
        self.set_pred_reg(range, reg);
        self.set_bit(not_bit, not ^ src_mod_is_bnot(src.src_mod));
    }

    fn set_pred_src(&mut self, range: Range<usize>, not_bit: usize, src: &Src) {
        self.set_pred_src_file(range, not_bit, src, RegFile::Pred);
    }

    fn set_upred_src(
        &mut self,
        range: Range<usize>,
        not_bit: usize,
        src: &Src,
    ) {
        self.set_pred_src_file(range, not_bit, src, RegFile::UPred);
    }

    fn set_rev_pred_src_file(
        &mut self,
        range: Range<usize>,
        not_bit: usize,
        src: &Src,
        file: RegFile,
    ) {
        let (not, reg) = match src.src_ref {
            SrcRef::True => (false, self.true_reg(file)),
            SrcRef::False => (true, self.true_reg(file)),
            SrcRef::Reg(reg) => {
                assert!(reg.file() == file);
                (false, reg)
            }
            _ => panic!("Not a register"),
        };

        assert!(range.len() == 3);
        assert!(reg.base_idx() <= 7);
        assert!(reg.comps() == 1);

        // These sources are funky.  They're encoded backwards.
        self.set_field(range, 7 - reg.base_idx());

        self.set_bit(not_bit, not ^ src_mod_is_bnot(src.src_mod));
    }

    fn set_rev_pred_src(
        &mut self,
        range: Range<usize>,
        not_bit: usize,
        src: &Src,
    ) {
        self.set_rev_pred_src_file(range, not_bit, src, RegFile::Pred)
    }

    fn set_rev_upred_src(
        &mut self,
        range: Range<usize>,
        not_bit: usize,
        src: &Src,
    ) {
        self.set_rev_pred_src_file(range, not_bit, src, RegFile::UPred)
    }

    fn set_src_cb(&mut self, range: Range<usize>, cx_bit: usize, cb: &CBufRef) {
        let mut v = BitMutView::new_subset(self, range);
        v.set_field(6..22, cb.offset);
        match cb.buf {
            CBuf::Binding(idx) => {
                v.set_field(22..27, idx);
                self.set_bit(cx_bit, false);
            }
            CBuf::BindlessUGPR(reg) => {
                assert!(reg.base_idx() <= 63);
                assert!(reg.file() == RegFile::UGPR);
                v.set_field(0..6, reg.base_idx());
                self.set_bit(cx_bit, true);
            }
            CBuf::BindlessSSA(_) => panic!("SSA values must be lowered"),
        }
    }

    fn set_pred(&mut self, pred: &Pred) {
        assert!(!pred.is_false());
        self.set_pred_reg(
            12..15,
            match pred.pred_ref {
                PredRef::None => self.true_reg(RegFile::Pred),
                PredRef::Reg(reg) => reg,
                PredRef::SSA(_) => panic!("SSA values must be lowered"),
            },
        );
        self.set_bit(15, pred.pred_inv);
    }

    fn set_dst(&mut self, dst: &Dst) {
        match dst {
            Dst::None => self.set_reg(16..24, self.zero_reg(RegFile::GPR)),
            Dst::Reg(reg) => self.set_reg(16..24, *reg),
            _ => panic!("Not a register"),
        }
    }

    fn set_udst(&mut self, dst: &Dst) {
        match dst {
            Dst::None => self.set_ureg(16, self.zero_reg(RegFile::UGPR)),
            Dst::Reg(reg) => self.set_ureg(16, *reg),
            _ => panic!("Not a register"),
        }
    }

    fn set_bar_reg(&mut self, range: Range<usize>, reg: RegRef) {
        assert!(range.len() == 4);
        assert!(reg.file() == RegFile::Bar);
        assert!(reg.comps() == 1);
        self.set_field(range, reg.base_idx());
    }

    fn set_bar_dst(&mut self, range: Range<usize>, dst: &Dst) {
        self.set_bar_reg(range, *dst.as_reg().unwrap());
    }

    fn set_bar_src(&mut self, range: Range<usize>, src: &Src) {
        assert!(src.is_unmodified());
        self.set_bar_reg(range, *src.src_ref.as_reg().unwrap());
    }

    fn set_instr_deps(&mut self, deps: &InstrDeps) {
        self.set_field(105..109, deps.delay);
        self.set_field(110..113, deps.wr_bar().unwrap_or(7));
        self.set_field(113..116, deps.rd_bar().unwrap_or(7));
        self.set_field(116..122, deps.wt_bar_mask);
        if self.sm < 120 {
            self.set_bit(109, deps.yld);
            self.set_field(122..126, deps.reuse_mask);
        }
    }
}

//
// Helpers for encoding of ALU instructions
//

struct ALURegRef {
    pub reg: RegRef,
    pub abs: bool,
    pub neg: bool,
    pub swizzle: SrcSwizzle,
}

struct ALUCBufRef {
    pub cb: CBufRef,
    pub abs: bool,
    pub neg: bool,
    pub swizzle: SrcSwizzle,
}

enum ALUSrc {
    None,
    Imm32(u32),
    Reg(ALURegRef),
    UReg(ALURegRef),
    CBuf(ALUCBufRef),
}

fn src_is_zero_or_gpr(src: &Src) -> bool {
    match src.src_ref {
        SrcRef::Zero => true,
        SrcRef::Reg(reg) => reg.file() == RegFile::GPR,
        _ => false,
    }
}

fn src_mod_has_abs(src_mod: SrcMod) -> bool {
    match src_mod {
        SrcMod::None | SrcMod::FNeg | SrcMod::INeg | SrcMod::BNot => false,
        SrcMod::FAbs | SrcMod::FNegAbs => true,
    }
}

fn src_mod_has_neg(src_mod: SrcMod) -> bool {
    match src_mod {
        SrcMod::None | SrcMod::FAbs => false,
        SrcMod::FNeg | SrcMod::FNegAbs | SrcMod::INeg | SrcMod::BNot => true,
    }
}

fn src_mod_is_bnot(src_mod: SrcMod) -> bool {
    match src_mod {
        SrcMod::None => false,
        SrcMod::BNot => true,
        _ => panic!("Not an predicate source modifier"),
    }
}

fn dst_is_bar(dst: &Dst) -> bool {
    match dst {
        Dst::None => false,
        Dst::SSA(ssa) => ssa.file() == RegFile::Bar,
        Dst::Reg(reg) => reg.file() == RegFile::Bar,
    }
}

impl ALUSrc {
    fn from_src(
        e: &SM70Encoder<'_>,
        src: Option<&Src>,
        op_is_uniform: bool,
    ) -> ALUSrc {
        let Some(src) = src else {
            return ALUSrc::None;
        };

        match &src.src_ref {
            SrcRef::Zero | SrcRef::Reg(_) => {
                let reg = match src.src_ref {
                    SrcRef::Zero => {
                        let file = if op_is_uniform {
                            RegFile::UGPR
                        } else {
                            RegFile::GPR
                        };
                        e.zero_reg(file)
                    }
                    SrcRef::Reg(reg) => reg,
                    _ => panic!("Invalid source ref"),
                };
                assert!(reg.comps() <= 2);
                let alu_ref = ALURegRef {
                    reg: reg,
                    abs: src_mod_has_abs(src.src_mod),
                    neg: src_mod_has_neg(src.src_mod),
                    swizzle: src.src_swizzle,
                };
                if op_is_uniform {
                    assert!(reg.file() == RegFile::UGPR);
                    ALUSrc::Reg(alu_ref)
                } else {
                    match reg.file() {
                        RegFile::GPR => ALUSrc::Reg(alu_ref),
                        RegFile::UGPR => ALUSrc::UReg(alu_ref),
                        _ => panic!("Invalid ALU register file"),
                    }
                }
            }
            SrcRef::Imm32(i) => {
                assert!(src.is_unmodified());
                assert!(src.src_swizzle.is_none());
                ALUSrc::Imm32(*i)
            }
            SrcRef::CBuf(cb) => {
                let alu_ref = ALUCBufRef {
                    cb: cb.clone(),
                    abs: src_mod_has_abs(src.src_mod),
                    neg: src_mod_has_neg(src.src_mod),
                    swizzle: src.src_swizzle,
                };
                ALUSrc::CBuf(alu_ref)
            }
            _ => panic!("Invalid ALU source"),
        }
    }
}

impl OffsetStride {
    fn encode_sm75(&self) -> u8 {
        match self {
            Self::X1 => 0,
            Self::X4 => 1,
            Self::X8 => 2,
            Self::X16 => 3,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum SM70SrcType {
    F16,
    F16v2,
    Other,
}

impl From<SrcType> for SM70SrcType {
    fn from(value: SrcType) -> Self {
        match value {
            SrcType::F16 => SM70SrcType::F16,
            SrcType::F16v2 => SM70SrcType::F16v2,
            _ => SM70SrcType::Other,
        }
    }
}

impl SM70SrcType {
    fn is_fp16(self) -> bool {
        self != Self::Other
    }
}

impl SM70Encoder<'_> {
    fn set_swizzle(
        &mut self,
        range: Range<usize>,
        swizzle: SrcSwizzle,
        src_type: SM70SrcType,
    ) {
        assert!(range.len() == 2);

        self.set_field(
            range,
            match (src_type, swizzle) {
                (_, SrcSwizzle::None) => 0x00_u8,
                (SM70SrcType::F16, SrcSwizzle::Xx) => 0x00_u8,
                (SM70SrcType::F16, SrcSwizzle::Yy) => 0x01_u8,
                (SM70SrcType::F16v2, SrcSwizzle::Xx) => 0x02_u8,
                (SM70SrcType::F16v2, SrcSwizzle::Yy) => 0x03_u8,
                _ => unreachable!("unsupported {swizzle:?} for {src_type:?}"),
            },
        );
    }

    fn set_alu_reg(
        &mut self,
        range: Range<usize>,
        abs_bit: usize,
        neg_bit: usize,
        swizzle_range: Range<usize>,
        file: RegFile,
        src_type: SM70SrcType,
        reg: &ALURegRef,
    ) {
        match file {
            RegFile::GPR => self.set_reg(range, reg.reg),
            RegFile::UGPR => self.set_ureg(range.start, reg.reg),
            _ => panic!("Invalid ALU src register file"),
        }

        self.set_bit(abs_bit, reg.abs);
        self.set_bit(neg_bit, reg.neg);

        if src_type.is_fp16() {
            self.set_swizzle(swizzle_range, reg.swizzle, src_type);
        } else {
            assert!(reg.swizzle == SrcSwizzle::None);
        }
    }

    fn encode_alu_src0(
        &mut self,
        src: &ALUSrc,
        file: RegFile,
        src_type: SM70SrcType,
    ) {
        let reg = match src {
            ALUSrc::None => return,
            ALUSrc::Reg(reg) => reg,
            _ => panic!("Invalid ALU src"),
        };
        self.set_alu_reg(24..32, 73, 72, 74..76, file, src_type, reg);
    }

    fn encode_alu_src2(
        &mut self,
        src: &ALUSrc,
        file: RegFile,
        src_type: SM70SrcType,
    ) {
        let reg = match src {
            ALUSrc::None => return,
            ALUSrc::Reg(reg) => reg,
            _ => panic!("Invalid ALU src"),
        };
        self.set_alu_reg(
            64..72,
            if src_type.is_fp16() { 83 } else { 74 },
            if src_type.is_fp16() { 84 } else { 75 },
            81..83,
            file,
            src_type,
            reg,
        );
    }

    fn encode_alu_reg(&mut self, reg: &ALURegRef, src_type: SM70SrcType) {
        self.set_alu_reg(32..40, 62, 63, 60..62, RegFile::GPR, src_type, reg);
    }

    fn encode_alu_ureg(&mut self, reg: &ALURegRef, src_type: SM70SrcType) {
        self.set_ureg(32, reg.reg);
        self.set_bit(62, reg.abs);
        self.set_bit(63, reg.neg);

        if src_type.is_fp16() {
            self.set_swizzle(60..62, reg.swizzle, src_type);
        } else {
            assert!(reg.swizzle == SrcSwizzle::None);
        }

        self.set_bit(91, true);
    }

    fn encode_alu_imm(&mut self, imm: &u32) {
        self.set_field(32..64, *imm);
    }

    fn encode_alu_cb(&mut self, cb: &ALUCBufRef, src_type: SM70SrcType) {
        self.set_src_cb(32..59, 91, &cb.cb);
        self.set_bit(62, cb.abs);
        self.set_bit(63, cb.neg);

        if src_type.is_fp16() {
            self.set_swizzle(60..62, cb.swizzle, src_type);
        } else {
            assert!(cb.swizzle == SrcSwizzle::None);
        }
    }

    fn encode_alu_base(
        &mut self,
        opcode: u16,
        dst: Option<&Dst>,
        src0: Option<&Src>,
        src1: Option<&Src>,
        src2: Option<&Src>,
        src_type: SM70SrcType,
    ) {
        if let Some(dst) = dst {
            self.set_dst(dst);
        }

        let src0 = ALUSrc::from_src(self, src0, false);
        let src1 = ALUSrc::from_src(self, src1, false);
        let src2 = ALUSrc::from_src(self, src2, false);

        self.encode_alu_src0(&src0, RegFile::GPR, src_type);

        let form = match &src2 {
            ALUSrc::None | ALUSrc::Reg(_) => {
                self.encode_alu_src2(&src2, RegFile::GPR, src_type);
                match &src1 {
                    ALUSrc::None => 1_u8, // form
                    ALUSrc::Reg(reg1) => {
                        self.encode_alu_reg(reg1, src_type);
                        1_u8 // form
                    }
                    ALUSrc::UReg(reg1) => {
                        self.encode_alu_ureg(reg1, src_type);
                        6_u8 // form
                    }
                    ALUSrc::Imm32(imm1) => {
                        self.encode_alu_imm(imm1);
                        4_u8 // form
                    }
                    ALUSrc::CBuf(cb1) => {
                        self.encode_alu_cb(cb1, src_type);
                        5_u8 // form
                    }
                }
            }
            ALUSrc::UReg(reg2) => {
                self.encode_alu_ureg(reg2, src_type);
                self.encode_alu_src2(&src1, RegFile::GPR, src_type);
                7_u8 // form
            }
            ALUSrc::Imm32(imm2) => {
                self.encode_alu_imm(imm2);
                self.encode_alu_src2(&src1, RegFile::GPR, src_type);
                2_u8 // form
            }
            ALUSrc::CBuf(cb2) => {
                // TODO set_src_cx
                self.encode_alu_cb(cb2, src_type);
                self.encode_alu_src2(&src1, RegFile::GPR, src_type);
                3_u8 // form
            }
        };

        self.set_field(0..9, opcode);
        self.set_field(9..12, form);
    }

    fn encode_alu(
        &mut self,
        opcode: u16,
        dst: Option<&Dst>,
        src0: Option<&Src>,
        src1: Option<&Src>,
        src2: Option<&Src>,
    ) {
        // The SrcType really only matters for FP16, so make this
        // convenient for all the other ops
        self.encode_alu_base(opcode, dst, src0, src1, src2, SM70SrcType::Other);
    }

    fn encode_fp16v2_alu(
        &mut self,
        opcode: u16,
        dst: Option<&Dst>,
        src0: Option<&Src>,
        src1: Option<&Src>,
        src2: Option<&Src>,
    ) {
        self.encode_alu_base(opcode, dst, src0, src1, src2, SM70SrcType::F16v2);
    }

    fn encode_ualu_base(
        &mut self,
        opcode: u16,
        dst: Option<&Dst>,
        src0: Option<&Src>,
        src1: Option<&Src>,
        src2: Option<&Src>,
        src_type: SM70SrcType,
    ) {
        if let Some(dst) = dst {
            self.set_udst(dst);
        }

        let src0 = ALUSrc::from_src(self, src0, true);
        let src1 = ALUSrc::from_src(self, src1, true);
        let src2 = ALUSrc::from_src(self, src2, true);

        // All uniform ALU requires bit 91 set
        self.set_bit(91, true);

        self.encode_alu_src0(&src0, RegFile::UGPR, src_type);
        let form = match &src2 {
            ALUSrc::None | ALUSrc::Reg(_) => {
                self.encode_alu_src2(&src2, RegFile::UGPR, src_type);
                match &src1 {
                    ALUSrc::None => 1_u8, // form
                    ALUSrc::Reg(reg1) => {
                        self.encode_alu_ureg(reg1, src_type);
                        1_u8 // form
                    }
                    ALUSrc::UReg(_) => panic!("UALU never has UReg"),
                    ALUSrc::Imm32(imm1) => {
                        self.encode_alu_imm(imm1);
                        4_u8 // form
                    }
                    ALUSrc::CBuf(_) => panic!("UALU does not support cbufs"),
                }
            }
            ALUSrc::UReg(_) => panic!("UALU never has UReg"),
            ALUSrc::Imm32(imm2) => {
                self.encode_alu_imm(imm2);
                self.encode_alu_src2(&src1, RegFile::UGPR, src_type);
                2_u8 // form
            }
            ALUSrc::CBuf(_) => panic!("UALU does not support cbufs"),
        };

        self.set_field(0..9, opcode);
        self.set_field(9..12, form);
    }

    fn encode_ualu(
        &mut self,
        opcode: u16,
        dst: Option<&Dst>,
        src0: Option<&Src>,
        src1: Option<&Src>,
        src2: Option<&Src>,
    ) {
        // The SrcType really only matters for FP16, so make this
        // convenient for all the other ops
        self.encode_ualu_base(opcode, dst, src0, src1, src2, SM70SrcType::Other)
    }

    fn set_rnd_mode(&mut self, range: Range<usize>, rnd_mode: FRndMode) {
        assert!(range.len() == 2);
        self.set_field(
            range,
            match rnd_mode {
                FRndMode::NearestEven => 0_u8,
                FRndMode::NegInf => 1_u8,
                FRndMode::PosInf => 2_u8,
                FRndMode::Zero => 3_u8,
            },
        );
    }
}

//
// Legalization helpers
//

fn op_gpr(op: &impl DstsAsSlice) -> RegFile {
    if op.is_uniform() {
        RegFile::UGPR
    } else {
        RegFile::GPR
    }
}

fn legalize_load_store_address(
    b: &mut LegalizeBuilder,
    addr: &mut Src,
    uniform_addr: &mut Src,
    stride: Option<&mut OffsetStride>,
) {
    let stride_x1_or_none = matches!(stride, Some(OffsetStride::X1) | None);
    if addr.is_ugpr_reg() {
        if stride_x1_or_none && uniform_addr.is_zero() {
            *uniform_addr = mem::replace(addr, Src::ZERO);
        } else {
            b.copy_src_if_uniform(addr);
        }
    }

    if uniform_addr.is_gpr_reg() {
        if addr.is_zero() {
            assert!(stride_x1_or_none);
            *addr = mem::replace(uniform_addr, Src::ZERO);
        } else {
            let uniform_ssa = uniform_addr.as_ssa().unwrap();
            let mut ssa = addr.as_ssa().unwrap();

            let addr_comps = ssa.comps();
            if let Some(stride) = stride {
                if *stride != OffsetStride::X1 {
                    assert_eq!(addr_comps, 1);
                    let shift = stride.shift();
                    let shift = b.copy(shift.into());
                    *addr = b.shl(addr.clone(), shift.into()).into();
                    ssa = addr.as_ssa().unwrap();
                    *stride = OffsetStride::X1;
                }
            }

            if uniform_ssa.comps() == 2 {
                // In case the non uniform address is 32 bits and the uniform one 64,
                // we need convert it to 64 bits.
                if uniform_ssa.comps() != addr_comps {
                    let zero = b.copy(0.into());
                    *addr = [ssa[0], zero].into();
                }
                *addr = b
                    .iadd64(addr.clone(), uniform_addr.clone(), Src::ZERO)
                    .into()
            } else {
                *addr =
                    b.iadd(addr.clone(), uniform_addr.clone(), Src::ZERO).into()
            }
            *uniform_addr = 0.into();
        }
    }
}

//
// Implementations of SM70Op for each op we support on Volta+
//

impl SM70Op for OpFAdd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if src_is_zero_or_gpr(&self.srcs[1]) {
            e.encode_alu(
                0x021,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            )
        } else {
            e.encode_alu(
                0x021,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&Src::ZERO),
                Some(&self.srcs[1]),
            )
        };
        e.set_bit(77, self.saturate);
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_bit(80, self.ftz);
    }
}

impl SM70Op for OpFFma {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x023,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&self.srcs[2]),
        );
        e.set_bit(76, self.dnz);
        e.set_bit(77, self.saturate);
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_bit(80, self.ftz);
    }
}

impl SM70Op for OpFMnMx {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x009,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&Src::ZERO),
        );
        e.set_pred_src(87..90, 90, &self.min);
        e.set_bit(80, self.ftz);
    }
}

impl SM70Op for OpFMul {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x020,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&Src::ZERO),
        );
        e.set_bit(76, self.dnz);
        e.set_bit(77, self.saturate);
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_bit(80, self.ftz);
        e.set_field(84..87, 0x4_u8); // TODO: PDIV
    }
}

impl SM70Encoder<'_> {
    fn set_float_cmp_op(&mut self, range: Range<usize>, op: FloatCmpOp) {
        assert!(range.len() == 4);
        self.set_field(
            range,
            match op {
                FloatCmpOp::OrdLt => 0x01_u8,
                FloatCmpOp::OrdEq => 0x02_u8,
                FloatCmpOp::OrdLe => 0x03_u8,
                FloatCmpOp::OrdGt => 0x04_u8,
                FloatCmpOp::OrdNe => 0x05_u8,
                FloatCmpOp::OrdGe => 0x06_u8,
                FloatCmpOp::UnordLt => 0x09_u8,
                FloatCmpOp::UnordEq => 0x0a_u8,
                FloatCmpOp::UnordLe => 0x0b_u8,
                FloatCmpOp::UnordGt => 0x0c_u8,
                FloatCmpOp::UnordNe => 0x0d_u8,
                FloatCmpOp::UnordGe => 0x0e_u8,
                FloatCmpOp::IsNum => 0x07_u8,
                FloatCmpOp::IsNan => 0x08_u8,
            },
        );
    }

    fn set_pred_set_op(&mut self, range: Range<usize>, op: PredSetOp) {
        assert!(range.len() == 2);
        self.set_field(
            range,
            match op {
                PredSetOp::And => 0_u8,
                PredSetOp::Or => 1_u8,
                PredSetOp::Xor => 2_u8,
            },
        );
    }

    fn set_int_cmp_op(&mut self, range: Range<usize>, op: IntCmpOp) {
        assert!(range.len() == 3);
        self.set_field(
            range,
            match op {
                IntCmpOp::False => 0_u8,
                IntCmpOp::True => 7_u8,
                IntCmpOp::Eq => 2_u8,
                IntCmpOp::Ne => 5_u8,
                IntCmpOp::Lt => 1_u8,
                IntCmpOp::Le => 3_u8,
                IntCmpOp::Gt => 4_u8,
                IntCmpOp::Ge => 6_u8,
            },
        );
    }
}

impl SM70Op for OpFSet {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.cmp_op = self.cmp_op.flip();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x00a,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            None,
        );
        e.set_float_cmp_op(76..80, self.cmp_op);
        e.set_bit(80, self.ftz);
        e.set_field(87..90, 0x7_u8); // TODO: src predicate
    }
}

impl SM70Op for OpFSetP {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.cmp_op = self.cmp_op.flip();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x00b,
            None,
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            None,
        );

        e.set_pred_set_op(74..76, self.set_op);
        e.set_float_cmp_op(76..80, self.cmp_op);
        e.set_bit(80, self.ftz);

        e.set_pred_dst(81..84, &self.dst);
        e.set_pred_dst(84..87, &Dst::None); // dst1

        e.set_pred_src(87..90, 90, &self.accum);
    }
}

impl SM70Op for OpFSwzAdd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F32);
        b.copy_alu_src_if_not_reg(src1, gpr, SrcType::F32);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x822);
        e.set_dst(&self.dst);

        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(64..72, &self.srcs[1]);

        let mut subop = 0x0_u8;

        for (i, swz_op) in self.ops.iter().enumerate() {
            let swz_op = match swz_op {
                FSwzAddOp::Add => 0,
                FSwzAddOp::SubRight => 2,
                FSwzAddOp::SubLeft => 1,
                FSwzAddOp::MoveLeft => 3,
            };

            subop |= swz_op << ((self.ops.len() - i - 1) * 2);
        }

        e.set_field(32..40, subop);

        e.set_tex_ndv(77, self.deriv_mode);
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_bit(80, self.ftz);
    }
}

impl SM70Op for OpMuFu {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu_base(
            0x108,
            Some(&self.dst),
            None,
            Some(&self.src),
            None,
            self.src_types()[0].into(),
        );

        if e.sm >= 90 {
            e.set_field(
                72..73,
                match self.op_type {
                    FloatType::F32 => 0u8,
                    FloatType::F16 => 1u8,
                    /* .bf16 => 2 */
                    FloatType::F64 => unreachable!(),
                },
            );
        } else {
            e.set_bit(73, self.op_type == FloatType::F16);
        }
        e.set_field(
            74..80,
            match self.op {
                MuFuOp::Cos => 0_u8,
                MuFuOp::Sin => 1_u8,
                MuFuOp::Exp2 => 2_u8,
                MuFuOp::Log2 => 3_u8,
                MuFuOp::Rcp => 4_u8,
                MuFuOp::Rsq => 5_u8,
                MuFuOp::Rcp64H => 6_u8,
                MuFuOp::Rsq64H => 7_u8,
                MuFuOp::Sqrt => 8_u8,
                MuFuOp::Tanh => 9_u8,
            },
        );
    }
}

impl SM70Op for OpDAdd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F64);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x029,
            Some(&self.dst),
            Some(&self.srcs[0]),
            None,
            Some(&self.srcs[1]),
        );
        e.set_rnd_mode(78..80, self.rnd_mode);
    }
}

impl SM70Op for OpDFma {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F64);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::F64);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x02b,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&self.srcs[2]),
        );
        e.set_rnd_mode(78..80, self.rnd_mode);
    }
}

impl SM70Op for OpDMul {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F64);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x028,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            None,
        );
        e.set_rnd_mode(78..80, self.rnd_mode);
    }
}

impl SM70Op for OpDSetP {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.cmp_op = self.cmp_op.flip();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F64);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if src_is_zero_or_gpr(&self.srcs[1]) {
            e.encode_alu(
                0x02a,
                None,
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            )
        } else {
            e.encode_alu(
                0x02a,
                None,
                Some(&self.srcs[0]),
                None,
                Some(&self.srcs[1]),
            )
        };

        e.set_pred_set_op(74..76, self.set_op);
        e.set_float_cmp_op(76..80, self.cmp_op);

        e.set_pred_dst(81..84, &self.dst);
        e.set_pred_dst(84..87, &Dst::None); /* dst1 */

        e.set_pred_src(87..90, 90, &self.accum);
    }
}

impl SM70Op for OpHAdd2 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F16v2);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if src_is_zero_or_gpr(&self.srcs[1]) {
            e.encode_fp16v2_alu(
                0x030,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            )
        } else {
            e.encode_fp16v2_alu(
                0x030,
                Some(&self.dst),
                Some(&self.srcs[0]),
                None,
                Some(&self.srcs[1]),
            )
        };

        e.set_bit(77, self.saturate);
        e.set_bit(78, self.f32);
        e.set_bit(80, self.ftz);
        e.set_bit(85, false); // .BF16_V2 (SM90+)
    }
}

impl SM70Op for OpHFma2 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F16v2);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::F16v2);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_fp16v2_alu(
            0x031,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&self.srcs[2]),
        );

        e.set_bit(76, self.dnz);
        e.set_bit(77, self.saturate);
        e.set_bit(78, self.f32);
        e.set_bit(79, false); // .RELU (SM86+)
        e.set_bit(80, self.ftz);
        e.set_bit(85, false); // .BF16_V2 (SM86+)
    }
}

impl SM70Op for OpHMul2 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F16v2);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_fp16v2_alu(
            0x032,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            None,
        );

        e.set_bit(76, self.dnz);
        e.set_bit(77, self.saturate);
        e.set_bit(78, false); // .F32 (SM70-SM75)
        e.set_bit(79, false); // .RELU (SM86+)
        e.set_bit(80, self.ftz);
        e.set_bit(85, false); // .BF16_V2 (SM90+)
    }
}

impl SM70Op for OpHSet2 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.cmp_op = self.cmp_op.flip();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F16v2);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if src_is_zero_or_gpr(&self.srcs[1]) {
            e.encode_fp16v2_alu(
                0x033,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            )
        } else {
            e.encode_fp16v2_alu(
                0x033,
                Some(&self.dst),
                Some(&self.srcs[0]),
                None,
                Some(&self.srcs[1]),
            )
        };

        e.set_bit(65, false); // .BF16_V2 (SM90+)
        e.set_pred_set_op(69..71, self.set_op);

        // This differentiate between integer and fp16 output
        e.set_bit(71, true); // .BF
        e.set_float_cmp_op(76..80, self.cmp_op);
        e.set_bit(80, self.ftz);

        e.set_pred_src(87..90, 90, &self.accum);
    }
}

impl SM70Op for OpHSetP2 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.cmp_op = self.cmp_op.flip();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F16v2);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if src_is_zero_or_gpr(&self.srcs[1]) {
            e.encode_fp16v2_alu(
                0x034,
                None,
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            )
        } else {
            e.encode_fp16v2_alu(
                0x034,
                None,
                Some(&self.srcs[0]),
                None,
                Some(&self.srcs[1]),
            )
        };

        e.set_bit(65, false); // .BF16_V2 (SM90+)
        e.set_pred_set_op(69..71, self.set_op);
        e.set_bit(71, self.horizontal); // .H_AND
        e.set_float_cmp_op(76..80, self.cmp_op);
        e.set_bit(80, self.ftz);

        e.set_pred_dst(81..84, &self.dsts[0]);
        e.set_pred_dst(84..87, &self.dsts[1]);

        e.set_pred_src(87..90, 90, &self.accum);
    }
}

impl SM70Op for OpHMnMx2 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::F16v2);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(e.sm >= 80);

        e.encode_fp16v2_alu(
            0x040,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            None,
        );

        // This differentiate between integer and fp16 output
        e.set_bit(78, false); // .F32 (SM86)
        e.set_bit(80, self.ftz);
        e.set_bit(81, false); // .NAN
        e.set_bit(82, false); // .XORSIGN
        e.set_bit(85, false); // .BF16_V2

        e.set_pred_src(87..90, 90, &self.min);
    }
}

impl SM70Op for OpBMsk {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.pos, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x09b,
                Some(&self.dst),
                Some(&self.pos),
                Some(&self.width),
                None,
            )
        } else {
            e.encode_alu(
                0x01b,
                Some(&self.dst),
                Some(&self.pos),
                Some(&self.width),
                None,
            )
        };

        e.set_bit(75, self.wrap);
    }
}

impl SM70Op for OpBRev {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(0x0be, Some(&self.dst), None, Some(&self.src), None)
        } else {
            e.encode_alu(0x101, Some(&self.dst), None, Some(&self.src), None)
        }
    }
}

impl SM70Op for OpFlo {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(0x0bd, Some(&self.dst), None, Some(&self.src), None)
        } else {
            e.encode_alu(0x100, Some(&self.dst), None, Some(&self.src), None)
        };
        e.set_pred_dst(81..84, &Dst::None);
        e.set_field(74..75, self.return_shift_amount as u8);
        e.set_field(73..74, self.signed as u8);
        let not_mod = matches!(self.src.src_mod, SrcMod::BNot);
        e.set_field(63..64, not_mod)
    }
}

impl SM70Op for OpIAbs {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(0x013, Some(&self.dst), None, Some(&self.src), None)
    }
}

impl SM70Op for OpIAdd3 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        swap_srcs_if_not_reg(src2, src1, gpr);
        if !src0.is_unmodified() && !src1.is_unmodified() {
            assert!(self.overflow[0].is_none());
            assert!(self.overflow[1].is_none());
            b.copy_alu_src_and_lower_ineg(src0, gpr, SrcType::I32);
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::I32);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::I32);
        if !self.overflow[0].is_none() || !self.overflow[1].is_none() {
            b.copy_alu_src_if_ineg_imm(src1, gpr, SrcType::I32);
            b.copy_alu_src_if_ineg_imm(src2, gpr, SrcType::I32);
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        // Hardware requires at least one of these be unmodified
        assert!(self.srcs[0].is_unmodified() || self.srcs[1].is_unmodified());

        if self.is_uniform() {
            e.encode_ualu(
                0x090,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            )
        } else {
            e.encode_alu(
                0x010,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            )
        };

        e.set_pred_src(87..90, 90, &false.into());
        e.set_pred_src(77..80, 80, &false.into());

        e.set_pred_dst(81..84, &self.overflow[0]);
        e.set_pred_dst(84..87, &self.overflow[1]);
    }
}

impl SM70Op for OpIAdd3X {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        swap_srcs_if_not_reg(src2, src1, gpr);
        if !src0.is_unmodified() && !src1.is_unmodified() {
            let val = b.alloc_ssa(gpr);
            let old_src0 = std::mem::replace(src0, val.into());
            b.push_op(OpIAdd3X {
                srcs: [Src::ZERO, old_src0, Src::ZERO],
                overflow: [Dst::None, Dst::None],
                dst: val.into(),
                carry: [false.into(), false.into()],
            });
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::B32);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::B32);
        if !self.is_uniform() {
            b.copy_src_if_upred(&mut self.carry[0]);
            b.copy_src_if_upred(&mut self.carry[1]);
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        // Hardware requires at least one of these be unmodified
        assert!(self.srcs[0].is_unmodified() || self.srcs[1].is_unmodified());

        if self.is_uniform() {
            e.encode_ualu(
                0x090,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            );

            e.set_upred_src(87..90, 90, &self.carry[0]);
            e.set_upred_src(77..80, 80, &self.carry[1]);
        } else {
            e.encode_alu(
                0x010,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            );

            e.set_pred_src(87..90, 90, &self.carry[0]);
            e.set_pred_src(77..80, 80, &self.carry[1]);
        }

        e.set_bit(74, true); // .X

        e.set_pred_dst(81..84, &self.overflow[0]);
        e.set_pred_dst(84..87, &self.overflow[1]);
    }
}

impl SM70Op for OpIDp4 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src_type0, src_type1] = &mut self.src_types;
        let [src0, src1, src2] = &mut self.srcs;
        if swap_srcs_if_not_reg(src0, src1, gpr) {
            std::mem::swap(src_type0, src_type1);
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
        b.copy_alu_src_if_ineg_imm(src1, gpr, SrcType::I32);
        b.copy_alu_src_if_not_reg(src2, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x026,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&self.srcs[2]),
        );

        e.set_bit(
            73,
            match self.src_types[0] {
                IntType::U8 => false,
                IntType::I8 => true,
                _ => panic!("Invalid DP4 source type"),
            },
        );
        e.set_bit(
            74,
            match self.src_types[1] {
                IntType::U8 => false,
                IntType::I8 => true,
                _ => panic!("Invalid DP4 source type"),
            },
        );
    }
}

impl SM70Op for OpIMad {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x0a4,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            )
        } else {
            e.encode_alu(
                0x024,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            )
        };
        e.set_pred_dst(81..84, &Dst::None);
        e.set_bit(73, self.signed);
    }
}

impl SM70Op for OpIMad64 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1, src2] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
        b.copy_alu_src_if_both_not_reg(src1, src2, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x0a5,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            )
        } else {
            e.encode_alu(
                0x025,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            )
        };
        e.set_pred_dst(81..84, &Dst::None);
        e.set_bit(73, self.signed);
    }
}

impl SM70Op for OpIMnMx {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        swap_srcs_if_not_reg(src0, src1, gpr);
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x017,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            None,
        );
        e.set_pred_src(87..90, 90, &self.min);
        e.set_bit(
            73,
            match self.cmp_type {
                IntCmpType::U32 => false,
                IntCmpType::I32 => true,
            },
        );
        if e.sm >= 120 {
            e.set_bit(74, false); // 64-bit
            e.set_pred_src(77..80, 80, &false.into());
            e.set_pred_dst(81..84, &Dst::None);
            e.set_pred_dst(84..87, &Dst::None);
        }
    }
}

impl SM70Op for OpISetP {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.cmp_op = self.cmp_op.flip();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
        if !self.is_uniform() {
            b.copy_src_if_upred(&mut self.low_cmp);
            b.copy_src_if_upred(&mut self.accum);
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x08c,
                None,
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            );

            e.set_upred_src(68..71, 71, &self.low_cmp);
            e.set_upred_src(87..90, 90, &self.accum);
        } else {
            e.encode_alu(
                0x00c,
                None,
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            );

            e.set_pred_src(68..71, 71, &self.low_cmp);
            e.set_pred_src(87..90, 90, &self.accum);
        }

        e.set_bit(72, self.ex);

        e.set_field(
            73..74,
            match self.cmp_type {
                IntCmpType::U32 => 0_u32,
                IntCmpType::I32 => 1_u32,
            },
        );
        e.set_pred_set_op(74..76, self.set_op);
        e.set_int_cmp_op(76..79, self.cmp_op);

        e.set_pred_dst(81..84, &self.dst);
        e.set_pred_dst(84..87, &Dst::None); // dst1
    }
}

impl SM70Op for OpLea {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.a, gpr, SrcType::ALU);
        if self.dst_high {
            b.copy_alu_src_if_both_not_reg(
                &self.b,
                &mut self.a_high,
                gpr,
                SrcType::ALU,
            );
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(self.a.src_mod == SrcMod::None);
        assert!(
            self.intermediate_mod == SrcMod::None
                || self.b.src_mod == SrcMod::None
        );

        let zero = 0.into();
        let c = if self.dst_high {
            Some(&self.a_high)
        } else {
            // TODO: On Ada and earlier, src2 is ignored if !dst_high. On
            // Blackwell+, it seems to do something.
            Some(&zero)
        };

        if self.is_uniform() {
            e.encode_ualu(
                0x091,
                Some(&self.dst),
                Some(&self.a),
                Some(&self.b),
                c,
            );
        } else {
            e.encode_alu(
                0x011,
                Some(&self.dst),
                Some(&self.a),
                Some(&self.b),
                c,
            );
        }

        e.set_bit(72, self.intermediate_mod.is_ineg());
        e.set_field(75..80, self.shift);
        e.set_bit(80, self.dst_high);
        e.set_pred_dst(81..84, &self.overflow);
        e.set_bit(74, false); // .X
    }
}

impl SM70Op for OpLeaX {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.a, gpr, SrcType::ALU);
        if self.dst_high {
            b.copy_alu_src_if_both_not_reg(
                &self.b,
                &mut self.a_high,
                gpr,
                SrcType::ALU,
            );
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(self.a.src_mod == SrcMod::None);
        assert!(
            self.intermediate_mod == SrcMod::None
                || self.b.src_mod == SrcMod::None
        );

        let c = if self.dst_high {
            Some(&self.a_high)
        } else {
            // TODO: On Ada and earlier, src2 is ignored if !dst_high. On
            // Blackwell+, it seems to do something.
            Some(&Src::ZERO)
        };

        if self.is_uniform() {
            e.encode_ualu(
                0x091,
                Some(&self.dst),
                Some(&self.a),
                Some(&self.b),
                c,
            );
            e.set_upred_src(87..90, 90, &self.carry);
        } else {
            e.encode_alu(
                0x011,
                Some(&self.dst),
                Some(&self.a),
                Some(&self.b),
                c,
            );
            e.set_pred_src(87..90, 90, &self.carry);
        }

        e.set_bit(72, self.intermediate_mod.is_bnot());
        e.set_field(75..80, self.shift);
        e.set_bit(80, self.dst_high);
        e.set_pred_dst(81..84, &self.overflow);
        e.set_bit(74, true); // .X
    }
}

fn src_as_lop_imm(src: &Src) -> Option<bool> {
    let x = match src.src_ref {
        SrcRef::Zero => false,
        SrcRef::True => true,
        SrcRef::False => false,
        SrcRef::Imm32(i) => {
            if i == 0 {
                false
            } else if i == !0 {
                true
            } else {
                return None;
            }
        }
        _ => return None,
    };
    Some(x ^ src.src_mod.is_bnot())
}

fn fold_lop_src(src: &Src, x: &mut u8) {
    if let Some(i) = src_as_lop_imm(src) {
        *x = if i { !0 } else { 0 };
    }
    if src.src_mod.is_bnot() {
        *x = !*x;
    }
}

impl SM70Op for OpLop3 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        // Fold constants and modifiers if we can
        self.op = LogicOp3::new_lut(&|mut x, mut y, mut z| {
            fold_lop_src(&self.srcs[0], &mut x);
            fold_lop_src(&self.srcs[1], &mut y);
            fold_lop_src(&self.srcs[2], &mut z);
            self.op.eval(x, y, z)
        });
        for src in &mut self.srcs {
            src.src_mod = SrcMod::None;
            if src_as_lop_imm(src).is_some() {
                src.src_ref = SrcRef::Zero;
            }
        }

        let [src0, src1, src2] = &mut self.srcs;
        if !src_is_reg(src0, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src0, src1);
            self.op = LogicOp3::new_lut(&|x, y, z| self.op.eval(y, x, z))
        }
        if !src_is_reg(src2, gpr) && src_is_reg(src1, gpr) {
            std::mem::swap(src2, src1);
            self.op = LogicOp3::new_lut(&|x, y, z| self.op.eval(x, z, y))
        }

        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
        b.copy_alu_src_if_not_reg(src2, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x092,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            );

            e.set_upred_src(87..90, 90, &SrcRef::False.into());
        } else {
            e.encode_alu(
                0x012,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                Some(&self.srcs[2]),
            );

            e.set_pred_src(87..90, 90, &SrcRef::False.into());
        }

        e.set_field(72..80, self.op.lut);
        e.set_bit(80, false); // .PAND
        e.set_field(81..84, 7_u32); // pred
    }
}

impl SM70Op for OpPopC {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(0x0bf, Some(&self.dst), None, Some(&self.src), None)
        } else {
            e.encode_alu(0x109, Some(&self.dst), None, Some(&self.src), None)
        };

        let not_mod = matches!(self.src.src_mod, SrcMod::BNot);
        e.set_field(63..64, not_mod);
    }
}

impl SM70Op for OpShf {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.low, gpr, SrcType::ALU);
        b.copy_alu_src_if_both_not_reg(
            &self.shift,
            &mut self.high,
            gpr,
            SrcType::ALU,
        );
        self.reduce_shift_imm();
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x099,
                Some(&self.dst),
                Some(&self.low),
                Some(&self.shift),
                Some(&self.high),
            )
        } else {
            e.encode_alu(
                0x019,
                Some(&self.dst),
                Some(&self.low),
                Some(&self.shift),
                Some(&self.high),
            )
        };

        e.set_field(
            73..75,
            match self.data_type {
                IntType::I64 => 0_u8,
                IntType::U64 => 1_u8,
                IntType::I32 => 2_u8,
                IntType::U32 => 3_u8,
                _ => panic!("Invalid shift data type"),
            },
        );
        e.set_bit(75, self.wrap);
        e.set_bit(76, self.right);
        e.set_bit(80, self.dst_high);
    }
}

impl SM70Op for OpF2F {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(!self.integer_rnd);

        let opcode = if self.src_type.bits() <= 32 && self.dst_type.bits() <= 32
        {
            0x104
        } else {
            0x110
        };

        e.encode_alu_base(
            opcode,
            Some(&self.dst),
            None,
            Some(&self.src),
            None,
            self.src_types()[0].into(),
        );

        e.set_field(75..77, (self.dst_type.bits() / 8).ilog2());
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_bit(80, self.ftz);
        e.set_field(84..86, (self.src_type.bits() / 8).ilog2());
    }
}

impl SM70Op for OpF2FP {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, _src1] = &mut self.srcs;
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x03e,
            Some(&self.dst),
            Some(&self.srcs[0]),
            Some(&self.srcs[1]),
            Some(&Src::ZERO),
        );

        // .MERGE_C behavior
        // Use src1 and src2, src0 is unused
        // src1 get converted and packed in the lower 16 bits of dest.
        // src2 lower or high 16 bits (decided by .H1 flag) get packed in the upper of dest.
        e.set_bit(78, false); // TODO: .MERGE_C
        e.set_bit(72, false); // .H1 (MERGE_C only)
        e.set_rnd_mode(79..81, self.rnd_mode);
    }
}

impl SM70Op for OpF2I {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        let opcode = if self.src_type.bits() <= 32 && self.dst_type.bits() <= 32
        {
            0x105
        } else {
            0x111
        };

        e.encode_alu_base(
            opcode,
            Some(&self.dst),
            None,
            Some(&self.src),
            None,
            self.src_types()[0].into(),
        );

        e.set_bit(72, self.dst_type.is_signed());
        e.set_field(75..77, (self.dst_type.bits() / 8).ilog2());
        e.set_bit(77, false); // NTZ
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_bit(80, self.ftz);
        e.set_field(84..86, (self.src_type.bits() / 8).ilog2());
    }
}

impl SM70Op for OpI2F {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.src_type.bits() <= 32 && self.dst_type.bits() <= 32 {
            e.encode_alu(0x106, Some(&self.dst), None, Some(&self.src), None)
        } else {
            e.encode_alu(0x112, Some(&self.dst), None, Some(&self.src), None)
        };

        e.set_field(60..62, 0_u8); // TODO: subop
        e.set_bit(74, self.src_type.is_signed());
        e.set_field(75..77, (self.dst_type.bits() / 8).ilog2());
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_field(84..86, (self.src_type.bits() / 8).ilog2());
    }
}

impl SM70Op for OpFRnd {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        let opcode = if self.src_type.bits() <= 32 && self.dst_type.bits() <= 32
        {
            0x107
        } else {
            0x113
        };

        e.encode_alu_base(
            opcode,
            Some(&self.dst),
            None,
            Some(&self.src),
            None,
            self.src_types()[0].into(),
        );

        e.set_field(84..86, (self.src_type.bits() / 8).ilog2());
        e.set_bit(80, self.ftz);
        e.set_rnd_mode(78..80, self.rnd_mode);
        e.set_field(75..77, (self.dst_type.bits() / 8).ilog2());
    }
}

impl SM70Op for OpMov {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.set_opcode(0xc82);
            e.set_udst(&self.dst);

            // umov is encoded like a non-uniform ALU op
            let src = ALUSrc::from_src(e, Some(&self.src), true);
            let form: u8 = match &src {
                ALUSrc::Reg(reg) => {
                    e.encode_alu_ureg(reg, SM70SrcType::Other);
                    0x6 // form
                }
                ALUSrc::Imm32(imm) => {
                    e.encode_alu_imm(imm);
                    0x4 // form
                }
                _ => panic!("Invalid umov src"),
            };
            e.set_field(9..12, form);
        } else {
            e.encode_alu(0x002, Some(&self.dst), None, Some(&self.src), None);
            e.set_field(72..76, self.quad_lanes);
        }
    }
}

impl SM70Op for OpPrmt {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        let [src0, src1] = &mut self.srcs;
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
        b.copy_alu_src_if_not_reg(src1, gpr, SrcType::ALU);
        self.reduce_sel_imm();
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x96,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.sel),
                Some(&self.srcs[1]),
            )
        } else {
            e.encode_alu(
                0x16,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.sel),
                Some(&self.srcs[1]),
            )
        };

        e.set_field(
            72..75,
            match self.mode {
                PrmtMode::Index => 0_u8,
                PrmtMode::Forward4Extract => 1_u8,
                PrmtMode::Backward4Extract => 2_u8,
                PrmtMode::Replicate8 => 3_u8,
                PrmtMode::EdgeClampLeft => 4_u8,
                PrmtMode::EdgeClampRight => 5_u8,
                PrmtMode::Replicate16 => 6_u8,
            },
        );
    }
}

impl SM70Op for OpSel {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        if !self.is_uniform() {
            b.copy_src_if_upred(&mut self.cond);
        }
        let [src0, src1] = &mut self.srcs;
        if swap_srcs_if_not_reg(src0, src1, gpr) {
            self.cond = self.cond.clone().bnot();
        }
        b.copy_alu_src_if_not_reg(src0, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x087,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            );

            e.set_upred_src(87..90, 90, &self.cond);
        } else {
            e.encode_alu(
                0x007,
                Some(&self.dst),
                Some(&self.srcs[0]),
                Some(&self.srcs[1]),
                None,
            );

            e.set_pred_src(87..90, 90, &self.cond);
        }
    }
}

impl SM70Op for OpSgxt {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.a, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.encode_ualu(
                0x09a,
                Some(&self.dst),
                Some(&self.a),
                Some(&self.bits),
                None,
            );
        } else {
            e.encode_alu(
                0x01a,
                Some(&self.dst),
                Some(&self.a),
                Some(&self.bits),
                None,
            );
        }
        e.set_bit(73, self.signed);
        e.set_bit(75, false); // .W (wrap vs clamp)
    }
}

impl SM70Op for OpShfl {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.src, gpr, SrcType::GPR);
        b.copy_alu_src_if_not_reg_or_imm(&mut self.lane, gpr, SrcType::ALU);
        b.copy_alu_src_if_not_reg_or_imm(&mut self.c, gpr, SrcType::ALU);
        self.reduce_lane_c_imm();
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(self.lane.is_unmodified());
        assert!(self.c.is_unmodified());

        match &self.lane.src_ref {
            SrcRef::Zero | SrcRef::Reg(_) => match &self.c.src_ref {
                SrcRef::Zero | SrcRef::Reg(_) => {
                    e.set_opcode(0x389);
                    e.set_reg_src(32..40, &self.lane);
                    e.set_reg_src(64..72, &self.c);
                }
                SrcRef::Imm32(imm_c) => {
                    e.set_opcode(0x589);
                    e.set_reg_src(32..40, &self.lane);
                    e.set_field(40..53, *imm_c);
                }
                _ => panic!("Invalid instruction form"),
            },
            SrcRef::Imm32(imm_lane) => match &self.c.src_ref {
                SrcRef::Zero | SrcRef::Reg(_) => {
                    e.set_opcode(0x989);
                    e.set_field(53..58, *imm_lane);
                    e.set_reg_src(64..72, &self.c);
                }
                SrcRef::Imm32(imm_c) => {
                    e.set_opcode(0xf89);
                    e.set_field(40..53, *imm_c);
                    e.set_field(53..58, *imm_lane);
                }
                _ => panic!("Invalid instruction form"),
            },
            _ => panic!("Invalid instruction form"),
        };

        e.set_dst(&self.dst);
        e.set_pred_dst(81..84, &self.in_bounds);
        e.set_reg_src(24..32, &self.src);
        e.set_field(
            58..60,
            match self.op {
                ShflOp::Idx => 0_u8,
                ShflOp::Up => 1_u8,
                ShflOp::Down => 2_u8,
                ShflOp::Bfly => 3_u8,
            },
        );
    }
}

impl SM70Op for OpPLop3 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        // Fold constants and modifiers if we can
        for lop in &mut self.ops {
            *lop = LogicOp3::new_lut(&|mut x, mut y, mut z| {
                fold_lop_src(&self.srcs[0], &mut x);
                fold_lop_src(&self.srcs[1], &mut y);
                fold_lop_src(&self.srcs[2], &mut z);
                lop.eval(x, y, z)
            });
        }
        for src in &mut self.srcs {
            src.src_mod = SrcMod::None;
            if src_as_lop_imm(src).is_some() {
                src.src_ref = SrcRef::True;
            }
        }

        if !self.is_uniform() {
            // The warp form of plop3 allows a single uniform predicate in
            // src2. If we have a uniform predicate anywhere, try to move it
            // there.
            let [src0, src1, src2] = &mut self.srcs;
            if src_is_upred_reg(src0) && !src_is_upred_reg(src2) {
                std::mem::swap(src0, src2);
                for lop in &mut self.ops {
                    *lop = LogicOp3::new_lut(&|x, y, z| lop.eval(z, y, x))
                }
            }
            if src_is_upred_reg(src1) && !src_is_upred_reg(src2) {
                std::mem::swap(src1, src2);
                for lop in &mut self.ops {
                    *lop = LogicOp3::new_lut(&|x, y, z| lop.eval(x, z, y))
                }
            }
            b.copy_src_if_upred(src0);
            b.copy_src_if_upred(src1);
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.set_opcode(0x89c);

            e.set_upred_src(68..71, 71, &self.srcs[2]);
            e.set_upred_src(77..80, 80, &self.srcs[1]);
            e.set_upred_src(87..90, 90, &self.srcs[0]);
        } else {
            e.set_opcode(0x81c);

            if self.srcs[2]
                .src_ref
                .as_reg()
                .is_some_and(|r| r.is_uniform())
            {
                e.set_upred_src(68..71, 71, &self.srcs[2]);
                e.set_bit(67, true);
            } else {
                e.set_pred_src(68..71, 71, &self.srcs[2]);
            }
            e.set_pred_src(77..80, 80, &self.srcs[1]);
            e.set_pred_src(87..90, 90, &self.srcs[0]);
        }
        e.set_field(16..24, self.ops[1].lut);
        e.set_field(64..67, self.ops[0].lut & 0x7);
        e.set_field(72..77, self.ops[0].lut >> 3);

        e.set_pred_dst(81..84, &self.dsts[0]);
        e.set_pred_dst(84..87, &self.dsts[1]);
    }
}

impl SM70Op for OpR2UR {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if e.sm >= 100 {
            e.set_opcode(0x2ca);
        } else {
            e.set_opcode(0x3c2);
        }
        e.set_udst(&self.dst);
        e.set_reg_src(24..32, &self.src);
        e.set_pred_dst(81..84, &Dst::None);
    }
}

impl SM70Op for OpRedux {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_alu_src_if_not_reg(&mut self.src, RegFile::GPR, SrcType::GPR);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x3c4);
        e.set_udst(&self.dst);
        e.set_reg_src(24..32, &self.src);

        e.set_bit(
            73,
            match self.op {
                ReduxOp::Min(cmp_type) | ReduxOp::Max(cmp_type) => {
                    cmp_type == IntCmpType::I32
                }
                _ => false,
            },
        );
        e.set_field(
            78..81,
            match self.op {
                ReduxOp::And => 0_u8,
                ReduxOp::Or => 1,
                ReduxOp::Xor => 2,
                ReduxOp::Sum => 3,
                ReduxOp::Min(_) => 4,
                ReduxOp::Max(_) => 5,
            },
        );
    }
}

impl SM70Encoder<'_> {
    fn set_tex_cb_ref(&mut self, range: Range<usize>, cb: TexCBufRef) {
        assert!(range.len() == 19);
        let mut v = BitMutView::new_subset(self, range);
        assert!(cb.offset % 4 == 0);
        v.set_field(0..14, cb.offset / 4);
        v.set_field(14..19, cb.idx);
    }

    fn set_tex_dim(&mut self, range: Range<usize>, dim: TexDim) {
        assert!(range.len() == 3);
        self.set_field(
            range,
            match dim {
                TexDim::_1D => 0_u8,
                TexDim::Array1D => 4_u8,
                TexDim::_2D => 1_u8,
                TexDim::Array2D => 5_u8,
                TexDim::_3D => 2_u8,
                TexDim::Cube => 3_u8,
                TexDim::ArrayCube => 7_u8,
            },
        );
    }

    fn set_tex_lod_mode(&mut self, range: Range<usize>, lod_mode: TexLodMode) {
        assert!(range.len() == 3);
        assert!(self.sm <= 100);

        self.set_field(
            range,
            match lod_mode {
                TexLodMode::Auto => 0_u8,
                TexLodMode::Zero => 1_u8,
                TexLodMode::Bias => 2_u8,
                TexLodMode::Lod => 3_u8,
                TexLodMode::Clamp => 4_u8,
                TexLodMode::BiasClamp => 5_u8,
            },
        );
    }

    fn set_tex_lod_mode2(
        &mut self,
        range1: Range<usize>,
        range2: Range<usize>,
        lod_mode: TexLodMode,
    ) {
        self.set_field2(
            range1,
            range2,
            match lod_mode {
                TexLodMode::Auto => 0_u8,
                TexLodMode::Zero => 1_u8,
                TexLodMode::Bias => 2_u8,
                TexLodMode::Lod => 3_u8,
                TexLodMode::Clamp => 4_u8,
                TexLodMode::BiasClamp => 5_u8,
            },
        );
    }

    fn set_tex_ndv(&mut self, bit: usize, deriv_mode: TexDerivMode) {
        let ndv = match deriv_mode {
            TexDerivMode::Auto => false,
            TexDerivMode::NonDivergent => true,
            _ => panic!("{deriv_mode} is not supported"),
        };
        self.set_bit(bit, ndv);
    }

    fn set_tex_deriv_mode(
        &mut self,
        range: Range<usize>,
        deriv_mode: TexDerivMode,
    ) {
        assert!(range.len() == 2);
        assert!(self.sm >= 100);
        self.set_field(
            range,
            match deriv_mode {
                TexDerivMode::Auto => 0_u8,
                TexDerivMode::NonDivergent => 1_u8,
                TexDerivMode::ForceDivergent => {
                    assert!(self.sm >= 100 && self.sm < 110);
                    2_u8
                }
                TexDerivMode::DerivXY => {
                    assert!(self.sm >= 120);
                    3_u8
                }
            },
        );
    }

    fn set_image_dim(&mut self, range: Range<usize>, dim: ImageDim) {
        assert!(range.len() == 3);
        self.set_field(
            range,
            match dim {
                ImageDim::_1D => 0_u8,
                ImageDim::_1DBuffer => 1_u8,
                ImageDim::_1DArray => 2_u8,
                ImageDim::_2D => 3_u8,
                ImageDim::_2DArray => 4_u8,
                ImageDim::_3D => 5_u8,
            },
        );
    }

    fn set_tex_channel_mask(
        &mut self,
        range: Range<usize>,
        channel_mask: ChannelMask,
    ) {
        self.set_field(range, channel_mask.to_bits());
    }

    fn set_image_channel_mask(
        &mut self,
        range: Range<usize>,
        channel_mask: ChannelMask,
    ) {
        assert!(
            channel_mask.to_bits() == 0x1
                || channel_mask.to_bits() == 0x3
                || channel_mask.to_bits() == 0xf
        );
        self.set_field(range, channel_mask.to_bits());
    }
}

fn legalize_tex_instr(op: &mut impl SrcsAsSlice, b: &mut LegalizeBuilder) {
    // Texture instructions have one or two sources.  When they have two, the
    // second one is optional and we can set rZ instead.
    let srcs = op.srcs_as_mut_slice();
    assert!(matches!(&srcs[0].src_ref, SrcRef::SSA(_)));
    if let SrcRef::SSA(ssa) = &mut srcs[0].src_ref {
        b.copy_ssa_ref_if_uniform(ssa);
    }
    if srcs.len() > 1 {
        debug_assert!(srcs.len() == 2);
        assert!(matches!(&srcs[1].src_ref, SrcRef::SSA(_) | SrcRef::Zero));
        if let SrcRef::SSA(ssa) = &mut srcs[1].src_ref {
            b.copy_ssa_ref_if_uniform(ssa);
        }
    }
}

impl SM70Op for OpTex {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_tex_instr(self, b);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.tex {
            TexRef::Bound(_) => {
                panic!("SM70+ doesn't have legacy bound textures");
            }
            TexRef::CBuf(cb) => {
                assert!(e.sm < 100);
                e.set_opcode(0xb60);
                e.set_tex_cb_ref(40..59, cb);
            }
            TexRef::Bindless => {
                if e.sm >= 100 {
                    e.set_opcode(0xd61);
                    e.set_bit(91, true);
                } else {
                    e.set_opcode(0x361);
                    e.set_bit(59, true); // .B
                }
            }
        }

        e.set_dst(&self.dsts[0]);
        if let Dst::Reg(reg) = self.dsts[1] {
            e.set_reg(64..72, reg);
        } else {
            e.set_field(64..72, 255_u8);
        }
        e.set_pred_dst(81..84, &self.fault);

        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        if e.sm >= 100 {
            e.set_ureg_src(40, &Src::ZERO); // handle
            e.set_ureg_src(48, &Src::ZERO); // offset
        }

        e.set_bit(60, self.scalar);
        e.set_tex_dim(61..64, self.dim);
        e.set_tex_channel_mask(72..76, self.channel_mask);
        if e.sm >= 100 {
            e.set_field(
                56..58,
                match self.offset_mode {
                    TexOffsetMode::None => 0_u8,
                    TexOffsetMode::AddOffI => 1_u8,
                    TexOffsetMode::PerPx => panic!("Illegal offset value"),
                },
            );
            e.set_tex_deriv_mode(76..78, self.deriv_mode);
        } else {
            e.set_bit(76, self.offset_mode == TexOffsetMode::AddOffI);
            e.set_tex_ndv(77, self.deriv_mode);
        }
        e.set_bit(78, self.z_cmpr);
        e.set_eviction_priority(&self.mem_eviction_priority);
        if e.sm >= 100 {
            e.set_tex_lod_mode2(59..60, 87..90, self.lod_mode);
        } else {
            e.set_tex_lod_mode(87..90, self.lod_mode);
        }
        e.set_bit(90, self.nodep);
    }
}

impl SM70Op for OpTld {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_tex_instr(self, b);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.tex {
            TexRef::Bound(_) => {
                panic!("SM70+ doesn't have legacy bound textures");
            }
            TexRef::CBuf(cb) => {
                assert!(e.sm < 100);
                e.set_opcode(0xb66);
                e.set_tex_cb_ref(40..59, cb);
            }
            TexRef::Bindless => {
                if e.sm >= 100 {
                    e.set_opcode(0xd67);
                    e.set_bit(91, true);
                } else {
                    e.set_opcode(0x367);
                    e.set_bit(59, true); // .B
                }
            }
        }

        e.set_dst(&self.dsts[0]);
        if let Dst::Reg(reg) = self.dsts[1] {
            e.set_reg(64..72, reg);
        } else {
            e.set_field(64..72, 255_u8);
        }
        e.set_pred_dst(81..84, &self.fault);

        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        if e.sm >= 100 {
            e.set_ureg_src(40, &Src::ZERO); // handle
            e.set_ureg_src(48, &Src::ZERO); // offset
        }

        if e.sm >= 100 {
            e.set_field(
                56..58,
                match self.offset_mode {
                    TexOffsetMode::None => 0_u8,
                    TexOffsetMode::AddOffI => 1_u8,
                    TexOffsetMode::PerPx => panic!("Illegal offset value"),
                },
            );
        } else {
            e.set_bit(76, self.offset_mode == TexOffsetMode::AddOffI);
        }
        e.set_bit(60, self.scalar);
        e.set_tex_dim(61..64, self.dim);
        e.set_tex_channel_mask(72..76, self.channel_mask);

        if e.sm >= 120 {
            // MS vs UMS
            e.set_bit(77, self.is_ms);
        } else {
            // bit 77: .CL
            e.set_bit(78, self.is_ms);
        }
        // bits 79..81: .F16
        e.set_eviction_priority(&self.mem_eviction_priority);
        assert!(self.lod_mode.is_explicit_lod());
        if e.sm >= 100 {
            e.set_tex_lod_mode2(59..60, 87..90, self.lod_mode);
        } else {
            e.set_tex_lod_mode(87..90, self.lod_mode);
        }
        e.set_bit(90, self.nodep);
    }
}

impl SM70Op for OpTld4 {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_tex_instr(self, b);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.tex {
            TexRef::Bound(_) => {
                panic!("SM70+ doesn't have legacy bound textures");
            }
            TexRef::CBuf(cb) => {
                assert!(e.sm < 100);
                e.set_opcode(0xb63);
                e.set_tex_cb_ref(40..59, cb);
            }
            TexRef::Bindless => {
                if e.sm >= 100 {
                    e.set_opcode(0xd64);
                    e.set_bit(91, true);
                } else {
                    e.set_opcode(0x364);
                    e.set_bit(59, true); // .B
                }
            }
        }

        e.set_dst(&self.dsts[0]);
        if let Dst::Reg(reg) = self.dsts[1] {
            e.set_reg(64..72, reg);
        } else {
            e.set_field(64..72, 255_u8);
        }
        e.set_pred_dst(81..84, &self.fault);

        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        if e.sm >= 100 {
            e.set_ureg_src(40, &Src::ZERO); // handle
            e.set_ureg_src(48, &Src::ZERO); // offset
        }

        e.set_bit(60, self.scalar);
        e.set_tex_dim(61..64, self.dim);
        e.set_tex_channel_mask(72..76, self.channel_mask);
        e.set_field(
            76..78,
            match self.offset_mode {
                TexOffsetMode::None => 0_u8,
                TexOffsetMode::AddOffI => 1_u8,
                TexOffsetMode::PerPx => 2_u8,
            },
        );
        // bit 77: .CL
        e.set_bit(78, self.z_cmpr);
        e.set_eviction_priority(&self.mem_eviction_priority);
        e.set_field(87..89, self.comp);
        e.set_bit(90, self.nodep);
    }
}

impl SM70Op for OpTmml {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_tex_instr(self, b);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.tex {
            TexRef::Bound(_) => {
                panic!("SM70+ doesn't have legacy bound textures");
            }
            TexRef::CBuf(cb) => {
                assert!(e.sm < 100);
                e.set_opcode(0xb69);
                e.set_tex_cb_ref(40..59, cb);
            }
            TexRef::Bindless => {
                e.set_opcode(0x36a);
                e.set_bit(59, true); // .B
            }
        }

        e.set_dst(&self.dsts[0]);
        if let Dst::Reg(reg) = self.dsts[1] {
            e.set_reg(64..72, reg);
        } else {
            e.set_field(64..72, 255_u8);
        }

        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        if e.sm >= 100 {
            e.set_ureg_src(40, &Src::ZERO); // handle
        }

        e.set_tex_dim(61..64, self.dim);
        e.set_tex_channel_mask(72..76, self.channel_mask);
        if e.sm >= 100 {
            e.set_tex_deriv_mode(76..78, self.deriv_mode);
        } else {
            e.set_tex_ndv(77, self.deriv_mode);
        }
        e.set_bit(90, self.nodep);
    }
}

impl SM70Op for OpTxd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_tex_instr(self, b);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.tex {
            TexRef::Bound(_) => {
                panic!("SM70+ doesn't have legacy bound textures");
            }
            TexRef::CBuf(cb) => {
                assert!(e.sm < 100);
                e.set_opcode(0xb6c);
                e.set_tex_cb_ref(40..59, cb);
            }
            TexRef::Bindless => {
                if e.sm >= 100 {
                    e.set_opcode(0xd6d);
                    e.set_bit(91, true);
                } else {
                    e.set_opcode(0x36d);
                    e.set_bit(59, true); // .B
                }
            }
        }

        e.set_dst(&self.dsts[0]);
        if let Dst::Reg(reg) = self.dsts[1] {
            e.set_reg(64..72, reg);
        } else {
            e.set_field(64..72, 255_u8);
        }
        e.set_pred_dst(81..84, &self.fault);

        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        if e.sm >= 100 {
            e.set_ureg_src(40, &Src::ZERO); // handle
            e.set_ureg_src(48, &Src::ZERO); // offset
        }

        if e.sm >= 100 {
            e.set_field(
                56..58,
                match self.offset_mode {
                    TexOffsetMode::None => 0_u8,
                    TexOffsetMode::AddOffI => 1_u8,
                    TexOffsetMode::PerPx => panic!("Illegal offset value"),
                },
            );
        } else {
            e.set_bit(76, self.offset_mode == TexOffsetMode::AddOffI);
        }
        e.set_tex_dim(61..64, self.dim);
        e.set_tex_channel_mask(72..76, self.channel_mask);

        e.set_eviction_priority(&self.mem_eviction_priority);
        e.set_bit(90, self.nodep);
    }
}

impl SM70Op for OpTxq {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_tex_instr(self, b);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.tex {
            TexRef::Bound(_) => {
                panic!("SM70+ doesn't have legacy bound textures");
            }
            TexRef::CBuf(cb) => {
                assert!(e.sm < 100);
                e.set_opcode(0xb6f);
                e.set_tex_cb_ref(40..59, cb);
            }
            TexRef::Bindless => {
                e.set_opcode(0x370);
                e.set_bit(59, true); // .B
            }
        }

        e.set_dst(&self.dsts[0]);
        if let Dst::Reg(reg) = self.dsts[1] {
            e.set_reg(64..72, reg);
        } else {
            e.set_field(64..72, 255_u8);
        }

        e.set_reg_src(24..32, &self.src);
        if e.sm >= 100 {
            e.set_ureg_src(40, &Src::ZERO); // handle
        }

        e.set_field(
            62..64,
            match self.query {
                TexQuery::Dimension => 0_u8,
                TexQuery::TextureType => 1_u8,
                TexQuery::SamplerPos => 2_u8,
            },
        );
        e.set_tex_channel_mask(72..76, self.channel_mask);
        e.set_bit(90, self.nodep);
    }
}

impl SM70Encoder<'_> {
    fn set_mem_order(&mut self, order: &MemOrder) {
        if self.sm < 80 {
            let scope = match order {
                MemOrder::Constant => MemScope::System,
                MemOrder::Weak => MemScope::CTA,
                MemOrder::Strong(s) => *s,
            };
            self.set_field(
                77..79,
                match scope {
                    MemScope::CTA => 0_u8,
                    // SM => 1_u8,
                    MemScope::GPU => 2_u8,
                    MemScope::System => 3_u8,
                },
            );
            self.set_field(
                79..81,
                match order {
                    MemOrder::Constant => 0_u8,
                    MemOrder::Weak => 1_u8,
                    MemOrder::Strong(_) => 2_u8,
                    // MMIO => 3_u8,
                },
            );
        } else {
            self.set_field(
                77..81,
                match order {
                    MemOrder::Constant => 0x4_u8,
                    MemOrder::Weak => 0x0_u8,
                    MemOrder::Strong(MemScope::CTA) => 0x5_u8,
                    MemOrder::Strong(MemScope::GPU) => 0x7_u8,
                    MemOrder::Strong(MemScope::System) => 0xa_u8,
                },
            );
        }
    }

    fn set_eviction_priority(&mut self, pri: &MemEvictionPriority) {
        self.set_field(
            84..87,
            match pri {
                MemEvictionPriority::First => 0_u8,
                MemEvictionPriority::Normal => 1_u8,
                MemEvictionPriority::Last => 2_u8,
                MemEvictionPriority::LastUse => 3_u8,
                MemEvictionPriority::Unchanged => 4_u8,
                MemEvictionPriority::NoAllocate => 5_u8,
            },
        );
    }

    fn set_mem_type(&mut self, range: Range<usize>, mem_type: MemType) {
        assert!(range.len() == 3);
        self.set_field(
            range,
            match mem_type {
                MemType::U8 => 0_u8,
                MemType::I8 => 1_u8,
                MemType::U16 => 2_u8,
                MemType::I16 => 3_u8,
                MemType::B32 => 4_u8,
                MemType::B64 => 5_u8,
                MemType::B128 => 6_u8,
            },
        );
    }

    fn set_mem_access(&mut self, access: &MemAccess) {
        self.set_mem_type(73..76, access.mem_type);
        self.set_mem_order(&access.order);
        self.set_eviction_priority(&access.eviction_priority);
    }
}

impl SM70Op for OpSuLd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.handle);
        b.copy_src_if_uniform(&mut self.coord);

        // suld.constant doesn't exist on Volta or Turing but it's always safe
        // to silently degrade to suld.weak
        if self.mem_order == MemOrder::Constant && b.sm() < 80 {
            self.mem_order = MemOrder::Weak;
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.image_access {
            ImageAccess::Binary(mem_type) => {
                e.set_opcode(0x99a);
                e.set_mem_type(73..76, mem_type);
            }
            ImageAccess::Formatted(channel_mask) => {
                e.set_opcode(0x998);
                e.set_image_channel_mask(72..76, channel_mask);
            }
        }

        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.coord);
        e.set_reg_src(64..72, &self.handle);
        e.set_pred_dst(81..84, &self.fault);
        if e.sm >= 120 {
            e.set_ureg_src(48, &Src::ZERO); // handle
        }

        e.set_image_dim(61..64, self.image_dim);
        e.set_mem_order(&self.mem_order);
        e.set_eviction_priority(&self.mem_eviction_priority);
    }
}

impl SM70Op for OpSuSt {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.handle);
        b.copy_src_if_uniform(&mut self.coord);
        b.copy_src_if_uniform(&mut self.data);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        match self.image_access {
            ImageAccess::Binary(mem_type) => {
                e.set_opcode(0x99e);
                e.set_mem_type(73..76, mem_type);
            }
            ImageAccess::Formatted(channel_mask) => {
                e.set_opcode(0x99c);
                e.set_image_channel_mask(72..76, channel_mask);
            }
        }

        e.set_reg_src(24..32, &self.coord);
        e.set_reg_src(32..40, &self.data);
        e.set_reg_src(64..72, &self.handle);
        if e.sm >= 120 {
            e.set_ureg_src(48, &Src::ZERO); // handle
        }

        e.set_image_dim(61..64, self.image_dim);
        e.set_mem_order(&self.mem_order);
        e.set_eviction_priority(&self.mem_eviction_priority);
    }
}

impl SM70Op for OpSuAtom {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.handle);
        b.copy_src_if_uniform(&mut self.coord);
        b.copy_src_if_uniform(&mut self.data);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.dst.is_none() {
            e.set_opcode(0x3a0);
            e.set_atom_op(87..90, self.atom_op);
        } else if let AtomOp::CmpExch(cmp_src) = self.atom_op {
            e.set_opcode(0x396);
            assert!(cmp_src == AtomCmpSrc::Packed);
        } else {
            e.set_opcode(0x394);
            e.set_atom_op(87..91, self.atom_op);
        };

        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.coord);
        e.set_reg_src(32..40, &self.data);
        e.set_reg_src(64..72, &self.handle);
        e.set_pred_dst(81..84, &self.fault);
        if e.sm >= 120 {
            e.set_ureg_src(48, &Src::ZERO); // handle
        }

        e.set_image_dim(61..64, self.image_dim);
        e.set_mem_order(&self.mem_order);
        e.set_eviction_priority(&self.mem_eviction_priority);

        e.set_bit(72, false); // .BA
        e.set_atom_type(self.atom_type, true);
    }
}

impl SM70Op for OpLd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_load_store_address(
            b,
            &mut self.addr,
            &mut self.uniform_addr,
            Some(&mut self.stride),
        );
        b.copy_src_if_uniform(&mut self.pred);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        let has_ugpr = e.sm >= 73;
        match self.access.space {
            MemSpace::Global(_) => {
                assert_eq!(self.stride, OffsetStride::X1);

                if has_ugpr {
                    e.set_opcode(0x981);
                    e.set_reg_addr(24..32, &self.addr, 90);
                    e.set_ureg_addr(32, &self.uniform_addr, 72);
                    e.set_rev_pred_src(64..67, 67, &self.pred);
                } else {
                    e.set_opcode(0x381);
                    e.set_reg_addr(24..32, &self.addr, 72);
                }

                e.set_pred_dst(81..84, &Dst::None);
                e.set_mem_access(&self.access);
            }
            MemSpace::Local => {
                assert!(self.pred.is_true());
                assert_eq!(self.stride, OffsetStride::X1);
                e.set_opcode(0x983);
                e.set_reg_src(24..32, &self.addr);
                if has_ugpr {
                    e.set_ureg_src(32, &self.uniform_addr);
                }
                e.set_field(84..87, 1_u8);

                e.set_mem_type(73..76, self.access.mem_type);
                assert!(self.access.order == MemOrder::Strong(MemScope::CTA));
                assert!(
                    self.access.eviction_priority
                        == MemEvictionPriority::Normal
                );
            }
            MemSpace::Shared => {
                e.set_opcode(0x984);
                assert!(self.pred.is_true());

                e.set_reg_src(24..32, &self.addr);
                if has_ugpr {
                    e.set_ureg_src(32, &self.uniform_addr);
                }
                e.set_mem_type(73..76, self.access.mem_type);
                assert!(self.access.order == MemOrder::Strong(MemScope::CTA));
                assert!(
                    self.access.eviction_priority
                        == MemEvictionPriority::Normal
                );

                assert!(e.sm >= 75 || self.stride == OffsetStride::X1);
                e.set_field(78..80, self.stride.encode_sm75());
                e.set_bit(87, false); // !.ZD - Returns a predicate?
            }
        }

        e.set_dst(&self.dst);
        e.set_field(40..64, self.offset);
        // We always enable UGPR mode, because the .E bit changes
        // which source it applies to depending on it.
        // This way it always applies to the UGPR.
        e.set_bit(91, has_ugpr);
    }
}

impl SM70Op for OpLdc {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.offset, gpr, SrcType::GPR);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        let SrcRef::CBuf(cb) = &self.cb.src_ref else {
            panic!("LDC must take a cbuf source");
        };

        match cb.buf {
            CBuf::Binding(idx) => {
                if self.is_uniform() {
                    if e.sm >= 100 {
                        e.set_opcode(0x7ac);
                        e.set_bit(91, true);
                        e.set_ureg_src(24, &self.offset);
                    } else {
                        e.set_opcode(0xab9);
                        e.set_bit(91, false);
                        assert!(self.offset.is_zero());
                    }
                    e.set_udst(&self.dst);
                    assert!(self.mode == LdcMode::Indexed);
                } else {
                    e.set_opcode(0xb82);
                    e.set_dst(&self.dst);

                    e.set_reg_src(24..32, &self.offset);
                    e.set_field(
                        78..80,
                        match self.mode {
                            LdcMode::Indexed => 0_u8,
                            LdcMode::IndexedLinear => 1_u8,
                            LdcMode::IndexedSegmented => 2_u8,
                            LdcMode::IndexedSegmentedLinear => 3_u8,
                        },
                    );
                    e.set_bit(91, false); // Bound
                }
                e.set_field(54..59, idx);
            }
            CBuf::BindlessUGPR(handle) => {
                if self.is_uniform() {
                    if e.sm >= 100 {
                        e.set_opcode(0xbac);
                    } else {
                        e.set_opcode(0xab9);
                    }
                    e.set_udst(&self.dst);

                    if e.sm >= 120 {
                        e.set_ureg_src(64, &self.offset);
                    } else if e.sm >= 100 {
                        // Blackwell A adds the source but it has to be zero
                        assert!(self.offset.is_zero());
                        e.set_ureg_src(64, &self.offset);
                    } else {
                        assert!(self.offset.is_zero());
                    }
                } else {
                    e.set_opcode(0x582);
                    e.set_dst(&self.dst);

                    e.set_reg_src(64..72, &self.offset);
                }

                e.set_ureg(24, handle);
                assert!(self.mode == LdcMode::Indexed);
                e.set_bit(91, true); // Bindless
            }
            CBuf::BindlessSSA(_) => panic!("SSA values must be lowered"),
        }

        if e.sm >= 100 && self.is_uniform() {
            e.set_field(37..54, cb.offset);
        } else {
            e.set_field(38..54, cb.offset);
        }
        e.set_mem_type(73..76, self.mem_type);

        if e.sm >= 120 {
            e.set_field(80..82, 0_u8); // tex/hdr_unpack
        } else if e.sm >= 100 {
            e.set_bit(80, false); // tex_unpack
        }
    }
}

impl SM70Op for OpSt {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.data);
        legalize_load_store_address(
            b,
            &mut self.addr,
            &mut self.uniform_addr,
            Some(&mut self.stride),
        );
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        let has_ugpr = e.sm >= 73;
        match self.access.space {
            MemSpace::Global(_) => {
                assert_eq!(self.stride, OffsetStride::X1);
                if has_ugpr {
                    e.set_opcode(0x986);
                    e.set_reg_addr(24..32, &self.addr, 90);
                    e.set_ureg_addr(64, &self.uniform_addr, 72);
                } else {
                    e.set_opcode(0x386);
                    e.set_reg_addr(24..32, &self.addr, 72);
                }
                e.set_mem_access(&self.access);
            }
            MemSpace::Local => {
                assert_eq!(self.stride, OffsetStride::X1);
                if has_ugpr {
                    e.set_opcode(0x987);
                    e.set_reg_src(24..32, &self.addr);
                    e.set_ureg_src(64, &self.uniform_addr);
                } else {
                    e.set_opcode(0x387);
                    e.set_reg_src(24..32, &self.addr);
                }
                e.set_field(84..87, 1_u8);

                e.set_mem_type(73..76, self.access.mem_type);
                assert!(self.access.order == MemOrder::Strong(MemScope::CTA));
                assert!(
                    self.access.eviction_priority
                        == MemEvictionPriority::Normal
                );
            }
            MemSpace::Shared => {
                if has_ugpr {
                    e.set_opcode(0x988);
                    e.set_reg_src(24..32, &self.addr);
                    e.set_ureg_src(64, &self.uniform_addr);
                } else {
                    e.set_opcode(0x388);
                    e.set_reg_src(24..32, &self.addr);
                }

                e.set_mem_type(73..76, self.access.mem_type);
                assert!(self.access.order == MemOrder::Strong(MemScope::CTA));
                assert!(
                    self.access.eviction_priority
                        == MemEvictionPriority::Normal
                );

                assert!(e.sm >= 75 || self.stride == OffsetStride::X1);
                e.set_field(78..80, self.stride.encode_sm75());
            }
        }

        e.set_reg_src(32..40, &self.data);
        e.set_field(40..64, self.offset);
        // We always enable UGPR mode, because the .E bit changes
        // which source it applies to depending on it.
        // This way it always applies to the UGPR.
        e.set_bit(91, has_ugpr);
    }
}

impl SM70Encoder<'_> {
    fn set_atom_op(&mut self, range: Range<usize>, atom_op: AtomOp) {
        self.set_field(
            range,
            match atom_op {
                AtomOp::Add => 0_u8,
                AtomOp::Min => 1_u8,
                AtomOp::Max => 2_u8,
                AtomOp::Inc => 3_u8,
                AtomOp::Dec => 4_u8,
                AtomOp::And => 5_u8,
                AtomOp::Or => 6_u8,
                AtomOp::Xor => 7_u8,
                AtomOp::Exch => 8_u8,
                AtomOp::CmpExch(_) => panic!("CmpExch is a separate opcode"),
            },
        );
    }

    fn set_atom_type(&mut self, atom_type: AtomType, su: bool) {
        if self.sm >= 90 && !su {
            // Float/int is differentiated by opcode
            self.set_field(
                73..77,
                match atom_type {
                    AtomType::F16x2 => 0_u8,
                    // f16x4 => 1
                    // f16x8 => 2
                    // bf16x2 => 3
                    // bf16x4 => 4
                    // bf16x8 => 5
                    AtomType::F32 => 9_u8, // .ftz
                    // f32x2.ftz => 10
                    // f32x4.ftz => 11
                    // f32x1 => 12
                    // f32x2 => 13
                    // f32x4 => 14
                    AtomType::F64 => 15_u8,

                    AtomType::U32 => 0,
                    AtomType::I32 => 1,
                    AtomType::U64 => 2,
                    AtomType::I64 => 3,
                    // u128 => 4,
                },
            );
        } else {
            self.set_field(
                73..76,
                match atom_type {
                    AtomType::U32 => 0_u8,
                    AtomType::I32 => 1_u8,
                    AtomType::U64 => 2_u8,
                    AtomType::F32 => 3_u8,
                    AtomType::F16x2 => 4_u8,
                    AtomType::I64 => 5_u8,
                    AtomType::F64 => 6_u8,
                },
            );
        }
    }
}

impl SM70Op for OpAtom {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.data);

        if matches!(self.atom_op, AtomOp::CmpExch(_)) {
            b.copy_src_if_uniform(&mut self.addr);
            b.copy_src_if_uniform(&mut self.cmpr);
        } else {
            legalize_load_store_address(
                b,
                &mut self.addr,
                &mut self.uniform_address,
                Some(&mut self.addr_stride),
            );
        }
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        let has_ugpr = e.sm >= 73;
        match self.mem_space {
            MemSpace::Global(_) => {
                if self.dst.is_none() {
                    if e.sm >= 90 && self.atom_type.is_float() {
                        e.set_opcode(0x9a6);
                    } else {
                        e.set_opcode(0x98e);
                    }

                    e.set_reg_src(32..40, &self.data);
                    e.set_field(40..64, self.addr_offset);
                    e.set_atom_op(87..90, self.atom_op);
                    if has_ugpr {
                        e.set_reg_addr(24..32, &self.addr, 90);
                        e.set_ureg_addr(64, &self.uniform_address, 72);
                        e.set_bit(91, true);
                    } else {
                        e.set_reg_addr(24..32, &self.addr, 72);
                        assert!(self.uniform_address.is_zero());
                    }
                } else if let AtomOp::CmpExch(cmp_src) = self.atom_op {
                    e.set_opcode(0x3a9);

                    assert!(cmp_src == AtomCmpSrc::Separate);
                    assert!(self.uniform_address.is_zero());
                    e.set_reg_addr(24..32, &self.addr, 72);
                    e.set_reg_src(32..40, &self.cmpr);
                    e.set_field(40..64, self.addr_offset);
                    e.set_reg_src(64..72, &self.data);
                    e.set_pred_dst(81..84, &Dst::None);
                } else {
                    if e.sm >= 90 && self.atom_type.is_float() {
                        e.set_opcode(0x9a3);
                    } else if has_ugpr {
                        e.set_opcode(0x9a8);
                    } else {
                        e.set_opcode(0x3a8);
                    }

                    if e.sm >= 100 {
                        e.set_reg_addr(24..32, &self.addr, 63);
                        e.set_ureg_addr(64, &self.uniform_address, 72);
                    } else if has_ugpr {
                        e.set_reg_addr(24..32, &self.addr, 70);
                        e.set_ureg_addr(64, &self.uniform_address, 72);
                    } else {
                        e.set_reg_addr(24..32, &self.addr, 72);
                        assert!(self.uniform_address.is_zero());
                    };

                    if e.sm >= 100 {
                        e.set_field(40..63, self.addr_offset);
                    } else {
                        e.set_field(40..64, self.addr_offset);
                    };

                    e.set_reg_src(32..40, &self.data);
                    e.set_pred_dst(81..84, &Dst::None);
                    e.set_atom_op(87..91, self.atom_op);
                    e.set_bit(91, has_ugpr);
                }

                e.set_mem_order(&self.mem_order);
                e.set_eviction_priority(&self.mem_eviction_priority);
                assert_eq!(self.addr_stride, OffsetStride::X1);
            }
            MemSpace::Local => panic!("Atomics do not support local"),
            MemSpace::Shared => {
                if let AtomOp::CmpExch(cmp_src) = self.atom_op {
                    e.set_opcode(0x38d);

                    assert!(cmp_src == AtomCmpSrc::Separate);
                    assert!(self.uniform_address.is_zero());
                    e.set_reg_src(32..40, &self.cmpr);
                    e.set_reg_src(64..72, &self.data);
                } else {
                    if has_ugpr {
                        e.set_opcode(0x98c);
                        e.set_ureg_src(64, &self.uniform_address);
                        e.set_bit(91, true);
                    } else {
                        e.set_opcode(0x38c);
                    }

                    e.set_reg_src(32..40, &self.data);
                    assert!(
                        self.atom_type != AtomType::U64
                            || self.atom_op == AtomOp::Exch,
                        "64-bit Shared atomics only support CmpExch or Exch"
                    );
                    assert!(
                        !self.atom_type.is_float(),
                        "Shared atomics don't support float"
                    );
                    e.set_atom_op(87..91, self.atom_op);
                }

                e.set_reg_src(24..32, &self.addr);
                e.set_field(40..64, self.addr_offset);
                assert!(e.sm >= 75 || self.addr_stride == OffsetStride::X1);
                e.set_field(78..80, self.addr_stride.encode_sm75());

                assert!(self.mem_order == MemOrder::Strong(MemScope::CTA));
                assert!(
                    self.mem_eviction_priority == MemEvictionPriority::Normal
                );
            }
        }

        e.set_dst(&self.dst);
        e.set_atom_type(self.atom_type, false);
    }
}

impl SM70Op for OpAL2P {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.offset);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x920);

        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.offset);

        e.set_field(40..50, self.addr);
        e.set_field(74..76, 0_u8); // comps
        e.set_bit(79, self.output);
    }
}

impl SM70Op for OpALd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.vtx);
        b.copy_src_if_uniform(&mut self.offset);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x321);

        e.set_dst(&self.dst);
        e.set_reg_src(32..40, &self.vtx);
        e.set_reg_src(24..32, &self.offset);

        e.set_field(40..50, self.addr);
        e.set_field(74..76, self.comps - 1);
        e.set_field(76..77, self.patch);
        e.set_field(77..78, self.phys);
        e.set_field(79..80, self.output);
    }
}

impl SM70Op for OpASt {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.data);
        b.copy_src_if_uniform(&mut self.vtx);
        b.copy_src_if_uniform(&mut self.offset);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x322);

        e.set_reg_src(32..40, &self.data);
        e.set_reg_src(64..72, &self.vtx);
        e.set_reg_src(24..32, &self.offset);

        e.set_field(40..50, self.addr);
        e.set_field(74..76, self.comps - 1);
        e.set_field(76..77, self.patch);
        e.set_field(77..78, self.phys);
    }
}

impl SM70Op for OpIpa {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.offset);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x326);

        e.set_dst(&self.dst);

        assert!(self.addr % 4 == 0);
        e.set_field(64..72, self.addr >> 2);

        e.set_field(
            76..78,
            match self.loc {
                InterpLoc::Default => 0_u8,
                InterpLoc::Centroid => 1_u8,
                InterpLoc::Offset => 2_u8,
            },
        );
        e.set_field(
            78..80,
            match self.freq {
                InterpFreq::Pass => 0_u8,
                InterpFreq::Constant => 1_u8,
                InterpFreq::State => 2_u8,
                InterpFreq::PassMulW => {
                    panic!("InterpFreq::PassMulW is invalid on SM70+");
                }
            },
        );

        assert!(self.inv_w.is_zero());
        e.set_reg_src(32..40, &self.offset);

        // TODO: What is this for?
        e.set_pred_dst(81..84, &Dst::None);
    }
}

impl SM70Op for OpLdTram {
    fn legalize(&mut self, _: &mut LegalizeBuilder) {}

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x3ad);
        e.set_dst(&self.dst);
        e.set_ureg(24, e.zero_reg(RegFile::UGPR));

        assert!(self.addr % 4 == 0);
        e.set_field(64..72, self.addr >> 2);

        e.set_bit(72, self.use_c);

        // Unknown but required
        e.set_bit(91, true);
    }
}

impl SM70Op for OpCCtl {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.addr);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(matches!(self.mem_space, MemSpace::Global(_)));
        e.set_opcode(0x98f);

        e.set_reg_src(24..32, &self.addr);
        e.set_field(32..64, self.addr_offset);

        e.set_field(
            87..91,
            match self.op {
                CCtlOp::PF1 => 0_u8,
                CCtlOp::PF2 => 1_u8,
                CCtlOp::WB => 2_u8,
                CCtlOp::IV => 3_u8,
                CCtlOp::IVAll => 4_u8,
                CCtlOp::RS => 5_u8,
                CCtlOp::IVAllP => 6_u8,
                CCtlOp::WBAll => 7_u8,
                CCtlOp::WBAllP => 8_u8,
                op => panic!("Unsupported cache control {op:?}"),
            },
        );
    }
}

impl SM70Op for OpMemBar {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x992);

        e.set_bit(72, false); // !.MMIO
        e.set_field(
            76..79,
            match self.scope {
                MemScope::CTA => 0_u8,
                // SM => 1_u8,
                MemScope::GPU => 2_u8,
                MemScope::System => 3_u8,
            },
        );
        e.set_bit(80, false); // .SC
    }
}

impl SM70Encoder<'_> {
    fn get_rel_offset(&mut self, label: &Label) -> i64 {
        let ip = u64::try_from(self.ip).unwrap();
        let ip = i64::try_from(ip).unwrap();

        let target_ip = *self.labels.get(label).unwrap();
        let target_ip = u64::try_from(target_ip).unwrap();
        let target_ip = i64::try_from(target_ip).unwrap();

        target_ip - ip - 4
    }

    fn set_rel_offset(&mut self, range: Range<usize>, label: &Label) {
        let rel_offset = self.get_rel_offset(label);
        self.set_field(range, rel_offset);
    }

    fn set_rel_offset2(
        &mut self,
        range1: Range<usize>,
        range2: Range<usize>,
        label: &Label,
    ) {
        let rel_offset = self.get_rel_offset(label);
        self.set_field2(range1, range2, rel_offset);
    }
}

impl SM70Op for OpBClear {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x355);

        e.set_dst(&Dst::None);
        e.set_bar_dst(24..28, &self.dst);

        e.set_bit(84, true); // .CLEAR
    }
}

impl SM70Op for OpBMov {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if dst_is_bar(&self.dst) {
            e.set_opcode(0x356);

            e.set_bar_dst(24..28, &self.dst);
            e.set_reg_src(32..40, &self.src);

            e.set_bit(84, self.clear);
        } else {
            e.set_opcode(0x355);

            e.set_dst(&self.dst);
            e.set_bar_src(24..28, &self.src);

            e.set_bit(84, self.clear);
        }
    }
}

impl SM70Op for OpBreak {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x942);
        assert!(self.bar_in.src_ref.as_reg() == self.bar_out.as_reg());
        e.set_bar_dst(16..20, &self.bar_out);
        e.set_pred_src(87..90, 90, &self.cond);
    }
}

impl SM70Op for OpBSSy {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x945);
        assert!(self.bar_in.src_ref.as_reg() == self.bar_out.as_reg());
        e.set_bar_dst(16..20, &self.bar_out);
        e.set_rel_offset(34..64, &self.target);
        e.set_pred_src(87..90, 90, &self.cond);
    }
}

impl SM70Op for OpBSync {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x941);
        e.set_bar_src(16..20, &self.bar);
        e.set_pred_src(87..90, 90, &self.cond);
    }
}

impl SM70Op for OpBra {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.cond.is_upred_reg() {
            assert!(e.sm >= 80);
            e.set_opcode(0x547);
            e.set_upred_src(24..27, 27, &self.cond);
            e.set_bit(32, true); // .U
            e.set_field(87..90, 0x7_u8);
            e.set_bit(91, true);
        } else {
            e.set_opcode(0x947);
            e.set_bit(32, false); // .U
            e.set_pred_src(87..90, 90, &self.cond);
        }

        if e.sm >= 100 {
            e.set_rel_offset2(16..24, 34..82, &self.target);
        } else {
            e.set_rel_offset(34..82, &self.target);
        }
    }
}

impl SM70Op for OpExit {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x94d);

        // ./.KEEPREFCOUNT/.PREEMPTED/.INVALID3
        e.set_field(84..85, false);
        e.set_field(85..86, false); // .NO_ATEXIT
        e.set_field(87..90, 0x7_u8); // TODO: Predicate
        e.set_field(90..91, false); // NOT
    }
}

impl SM70Op for OpWarpSync {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(0x148, None, None, Some(&Src::from(self.mask)), None);
        e.set_pred_src(87..90, 90, &SrcRef::True.into());
    }
}

impl SM70Op for OpBar {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0xb1d);

        // e.set_opcode(0x31d);

        // // src0 == src1
        // e.set_reg_src(32..40, SrcRef::Zero.into());

        // // 00: RED.POPC
        // // 01: RED.AND
        // // 02: RED.OR
        // e.set_field(74..76, 0_u8);

        // // 00: SYNC
        // // 01: ARV
        // // 02: RED
        // // 03: SCAN
        // e.set_field(77..79, 0_u8);

        // e.set_pred_src(87..90, 90, SrcRef::True.into());
    }
}

impl SM70Op for OpCS2R {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x805);
        e.set_dst(&self.dst);
        e.set_field(72..80, self.idx);
        e.set_bit(80, self.dst.as_reg().unwrap().comps() == 2); // .64
    }
}

impl SM70Op for OpIsberd {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.offset);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        // The immediate offset is only supported on SM86+
        assert!(e.sm >= 86 || self.imm_offset == 0);

        e.set_opcode(0x923);
        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.offset);
        e.set_field(40..56, self.imm_offset);
        e.set_field(
            74..76,
            match self.mem_type {
                MemType::U8 => 0_u8,
                MemType::U16 => 1_u8,
                MemType::B32 => 2_u8,
                _ => panic!("Invalid ISBERD mem type"),
            },
        );
        e.set_field(
            76..78,
            match self.access_type {
                IsbeAccessType::Map => 0_u8,
                IsbeAccessType::Patch => 1_u8,
                IsbeAccessType::Primitive => 2_u8,
                IsbeAccessType::Attribute => 3_u8,
            },
        );
        e.set_bit(78, self.skew);
        e.set_bit(79, self.output);
    }
}

impl SM70Op for OpIsbewr {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.offset);
        b.copy_src_if_uniform(&mut self.data);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(e.sm >= 75);

        // The immediate offset is only supported on SM86+
        assert!(e.sm >= 86 || self.imm_offset == 0);

        e.set_opcode(0x927);
        e.set_reg_src(24..32, &self.offset);
        e.set_reg_src(32..40, &self.data);
        e.set_field(40..56, self.imm_offset);
        e.set_field(
            74..76,
            match self.mem_type {
                MemType::U8 => 0_u8,
                MemType::U16 => 1_u8,
                MemType::B32 => 2_u8,
                _ => panic!("Invalid ISBEWR mem type"),
            },
        );
        e.set_field(
            76..78,
            match self.access_type {
                IsbeAccessType::Map => 0_u8,
                IsbeAccessType::Attribute => 3_u8,
                _ => panic!("Invalid ISBEWR access type"),
            },
        );
        e.set_bit(78, self.skew);
        e.set_bit(79, self.output);
    }
}

impl SM70Op for OpKill {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x95b);
        e.set_pred_src(87..90, 90, &SrcRef::True.into());
    }
}

impl SM70Op for OpNop {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x918);
    }
}

impl SM70Op for OpPixLd {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x925);
        e.set_dst(&self.dst);
        e.set_field(
            78..81,
            match &self.val {
                PixVal::MsCount => 0_u8,
                PixVal::CovMask => 1_u8,
                PixVal::CentroidOffset => 2_u8,
                PixVal::MyIndex => 3_u8,
                PixVal::InnerCoverage => 4_u8,
                other => panic!("Unsupported PixVal: {other}"),
            },
        );
        e.set_pred_dst(81..84, &Dst::None);
    }
}

impl SM70Op for OpS2R {
    fn legalize(&mut self, _b: &mut LegalizeBuilder) {
        // Nothing to do
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.set_opcode(0x9c3);
            e.set_udst(&self.dst);
        } else {
            e.set_opcode(0x919);
            e.set_dst(&self.dst);
        }
        e.set_field(72..80, self.idx);
    }
}

impl SM70Op for OpOut {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.handle, gpr, SrcType::GPR);
        b.copy_alu_src_if_not_reg_or_imm(&mut self.stream, gpr, SrcType::ALU);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x124,
            Some(&self.dst),
            Some(&self.handle),
            Some(&self.stream),
            None,
        );

        e.set_field(
            78..80,
            match self.out_type {
                OutType::Emit => 1_u8,
                OutType::Cut => 2_u8,
                OutType::EmitThenCut => 3_u8,
            },
        );
    }
}

impl SM70Op for OpOutFinal {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        let gpr = op_gpr(self);
        b.copy_alu_src_if_not_reg(&mut self.handle, gpr, SrcType::GPR);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.encode_alu(
            0x124,
            Some(&Dst::None),
            Some(&self.handle),
            Some(&Src::ZERO),
            None,
        );
    }
}

impl SM70Op for OpVote {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_upred(&mut self.pred);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        if self.is_uniform() {
            e.set_opcode(0x886);
            e.set_udst(&self.ballot);
        } else {
            e.set_opcode(0x806);
            e.set_dst(&self.ballot);
        }

        e.set_field(
            72..74,
            match self.op {
                VoteOp::All => 0_u8,
                VoteOp::Any => 1_u8,
                VoteOp::Eq => 2_u8,
            },
        );

        e.set_pred_dst(81..84, &self.vote);
        e.set_pred_src(87..90, 90, &self.pred);
    }
}

impl SM70Op for OpMatch {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.src);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        e.set_opcode(0x3a1);

        e.set_dst(&self.mask);
        e.set_reg_src(24..32, &self.src);
        e.set_bit(73, self.u64);

        e.set_bit(
            79,
            match self.op {
                MatchOp::Any => {
                    assert!(matches!(self.pred, Dst::None));
                    true
                }
                MatchOp::All => false,
            },
        );

        e.set_pred_dst(81..84, &self.pred);
    }
}

impl SM70Op for OpImma {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.srcs[0]);
        b.copy_src_if_uniform(&mut self.srcs[1]);
        b.copy_src_if_uniform(&mut self.srcs[2]);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(e.sm >= 75);

        e.set_opcode(0x237);
        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        e.set_reg_src(64..72, &self.srcs[2]);
        e.set_bit(74, true); // SRC1.COL

        if e.sm >= 90 {
            e.set_rev_upred_src(87..90, 90, &true.into());
        }

        assert!(self.mat_size == ImmaSize::M8N8K16 || e.sm >= 80);
        e.set_field2(
            75..76,
            85..87,
            match self.mat_size {
                ImmaSize::M8N8K16 => 0u8,
                ImmaSize::M8N8K32 => 2u8,
                ImmaSize::M16N8K16 => 4u8,
                ImmaSize::M16N8K32 => 5u8,
                ImmaSize::M16N8K64 => 6u8,
            },
        );

        e.set_bit(76, self.src_types[0].is_signed());
        e.set_bit(78, self.src_types[1].is_signed());
        e.set_bit(82, self.saturate);

        match self.mat_size {
            ImmaSize::M8N8K32 | ImmaSize::M16N8K64 => {
                assert_eq!(self.src_types[0].bits(), 4);
                assert_eq!(self.src_types[1].bits(), 4);
            }
            ImmaSize::M16N8K32 => {
                assert!(matches!(self.src_types[0].bits(), 4 | 8));
                assert!(matches!(self.src_types[1].bits(), 4 | 8));
            }
            ImmaSize::M8N8K16 | ImmaSize::M16N8K16 => {
                assert_eq!(self.src_types[0].bits(), 8);
                assert_eq!(self.src_types[1].bits(), 8);
            }
        }
        e.set_bit(83, self.src_types[0].bits() == 4);
        e.set_bit(84, self.src_types[1].bits() == 4);
    }
}

impl SM70Op for OpHmma {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.srcs[0]);
        b.copy_src_if_uniform(&mut self.srcs[1]);
        b.copy_src_if_uniform(&mut self.srcs[2]);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(e.sm >= 75);

        e.set_opcode(0x23c);
        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.srcs[0]);
        e.set_reg_src(32..40, &self.srcs[1]);
        e.set_reg_src(64..72, &self.srcs[2]);

        if e.sm >= 90 {
            e.set_rev_upred_src(87..90, 90, &true.into());
        }

        assert!(self.mat_size != HmmaSize::M16N8K4 || e.sm >= 80);
        e.set_field2(
            75..76,
            78..79,
            match self.mat_size {
                HmmaSize::M16N8K8 => 0u8,
                HmmaSize::M16N8K16 => 1u8,
                HmmaSize::M16N8K4 => 2u8,
            },
        );

        assert!(matches!(self.dst_type, FloatType::F16 | FloatType::F32));
        e.set_bit(76, self.dst_type == FloatType::F32);
        e.set_field(
            82..84,
            match self.src_type {
                FloatType::F16 => 0u8,
                // FloatType::BF16 => 1u8,
                // FloatType::TF32 => 2u8,
                _ => unreachable!("unsupported src type!"),
            },
        )
    }
}

impl SM70Op for OpLdsm {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        legalize_load_store_address(
            b,
            &mut self.addr,
            &mut self.uniform_addr,
            None,
        );
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(e.sm >= 75);

        e.set_opcode(0x83b);
        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.addr);
        e.set_ureg_src(32, &self.uniform_addr);
        e.set_field(40..64, self.offset);
        e.set_field(
            72..74,
            match self.mat_count {
                1 => 0u8,
                2 => 1u8,
                4 => 2u8,
                _ => panic!("Invalid LDSM mat count"),
            },
        );
        e.set_field(
            78..80,
            match self.mat_size {
                LdsmSize::M8N8 => 0u8,
                LdsmSize::MT8N8 => 1u8,
                // Those do value expansion and are weird, we'll probably never use them.
                // LdsmSize::M8N16 => 2,
                // LdsmSize::M8N32 => 3,
            },
        );
        e.set_bit(91, !self.uniform_addr.is_zero());
    }
}

impl SM70Op for OpMovm {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        b.copy_src_if_uniform(&mut self.src);
    }

    fn encode(&self, e: &mut SM70Encoder<'_>) {
        assert!(e.sm >= 75);

        e.set_opcode(0x23a);
        e.set_dst(&self.dst);
        e.set_reg_src(24..32, &self.src);
        // TODO: 1: M832, 2: M864
        e.set_field(78..80, 0); // MT88
    }
}

macro_rules! sm70_op_match {
    ($op: expr, |$x: ident| $y: expr) => {
        match $op {
            Op::FAdd($x) => $y,
            Op::FFma($x) => $y,
            Op::FMnMx($x) => $y,
            Op::FMul($x) => $y,
            Op::FSet($x) => $y,
            Op::FSetP($x) => $y,
            Op::FSwzAdd($x) => $y,
            Op::DAdd($x) => $y,
            Op::DFma($x) => $y,
            Op::DMul($x) => $y,
            Op::DSetP($x) => $y,
            Op::HAdd2($x) => $y,
            Op::HFma2($x) => $y,
            Op::HMul2($x) => $y,
            Op::HSet2($x) => $y,
            Op::HSetP2($x) => $y,
            Op::HMnMx2($x) => $y,
            Op::MuFu($x) => $y,
            Op::BMsk($x) => $y,
            Op::BRev($x) => $y,
            Op::Flo($x) => $y,
            Op::IAbs($x) => $y,
            Op::IAdd3($x) => $y,
            Op::IAdd3X($x) => $y,
            Op::IDp4($x) => $y,
            Op::IMad($x) => $y,
            Op::IMad64($x) => $y,
            Op::IMnMx($x) => $y,
            Op::ISetP($x) => $y,
            Op::Lea($x) => $y,
            Op::LeaX($x) => $y,
            Op::Lop3($x) => $y,
            Op::PopC($x) => $y,
            Op::Shf($x) => $y,
            Op::F2F($x) => $y,
            Op::F2FP($x) => $y,
            Op::F2I($x) => $y,
            Op::I2F($x) => $y,
            Op::FRnd($x) => $y,
            Op::Mov($x) => $y,
            Op::Movm($x) => $y,
            Op::Prmt($x) => $y,
            Op::Sel($x) => $y,
            Op::Sgxt($x) => $y,
            Op::Shfl($x) => $y,
            Op::PLop3($x) => $y,
            Op::R2UR($x) => $y,
            Op::Redux($x) => $y,
            Op::Tex($x) => $y,
            Op::Tld($x) => $y,
            Op::Tld4($x) => $y,
            Op::Tmml($x) => $y,
            Op::Txd($x) => $y,
            Op::Txq($x) => $y,
            Op::SuLd($x) => $y,
            Op::SuSt($x) => $y,
            Op::SuAtom($x) => $y,
            Op::Ld($x) => $y,
            Op::Ldc($x) => $y,
            Op::St($x) => $y,
            Op::Atom($x) => $y,
            Op::AL2P($x) => $y,
            Op::ALd($x) => $y,
            Op::ASt($x) => $y,
            Op::Ipa($x) => $y,
            Op::LdTram($x) => $y,
            Op::CCtl($x) => $y,
            Op::MemBar($x) => $y,
            Op::BClear($x) => $y,
            Op::BMov($x) => $y,
            Op::Break($x) => $y,
            Op::BSSy($x) => $y,
            Op::BSync($x) => $y,
            Op::Bra($x) => $y,
            Op::Exit($x) => $y,
            Op::WarpSync($x) => $y,
            Op::Bar($x) => $y,
            Op::CS2R($x) => $y,
            Op::Isberd($x) => $y,
            Op::Isbewr($x) => $y,
            Op::Kill($x) => $y,
            Op::Nop($x) => $y,
            Op::PixLd($x) => $y,
            Op::S2R($x) => $y,
            Op::Out($x) => $y,
            Op::OutFinal($x) => $y,
            Op::Vote($x) => $y,
            Op::Match($x) => $y,
            Op::Hmma($x) => $y,
            Op::Imma($x) => $y,
            Op::Ldsm($x) => $y,
            _ => panic!("Unsupported op: {}", $op),
        }
    };
}

impl SM70Op for Op {
    fn legalize(&mut self, b: &mut LegalizeBuilder) {
        sm70_op_match!(self, |op| op.legalize(b));
    }
    fn encode(&self, e: &mut SM70Encoder<'_>) {
        sm70_op_match!(self, |op| op.encode(e));
    }
}

pub fn legalize_sm70_op(
    _sm: &ShaderModel70,
    b: &mut LegalizeBuilder,
    op: &mut Op,
) {
    op.legalize(b);
}

pub fn encode_sm70_shader(sm: &ShaderModel70, s: &Shader<'_>) -> Vec<u32> {
    assert!(s.functions.len() == 1);
    let func = &s.functions[0];

    let mut ip = 0_usize;
    let mut labels = FxHashMap::default();
    for b in &func.blocks {
        labels.insert(b.label, ip);
        for instr in &b.instrs {
            if let Op::Nop(op) = &instr.op {
                if let Some(label) = op.label {
                    labels.insert(label, ip);
                }
            }
            ip += 4;
        }
    }

    let mut encoded = Vec::new();
    for b in &func.blocks {
        for instr in &b.instrs {
            let mut e = SM70Encoder {
                sm: sm.sm(),
                ip: encoded.len(),
                labels: &labels,
                inst: [0_u32; 4],
            };
            instr.op.encode(&mut e);
            e.set_pred(&instr.pred);
            e.set_instr_deps(&instr.deps);
            encoded.extend_from_slice(&e.inst[..]);
        }
    }
    encoded
}
