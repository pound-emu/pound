// Barrier instructions
INST(DMB,           "DMB",                 "1111010101111111111100000101oooo") // v7
INST(DSB,           "DSB",                 "1111010101111111111100000100oooo") // v7
INST(ISB,           "ISB",                 "1111010101111111111100000110oooo") // v7

// Branch instructions
INST(BLX_imm,       "BLX (imm)",           "1111101hvvvvvvvvvvvvvvvvvvvvvvvv") // v5
INST(BLX_reg,       "BLX (reg)",           "cccc000100101111111111110011mmmm") // v5
INST(B,             "B",                   "cccc1010vvvvvvvvvvvvvvvvvvvvvvvv") // v1
INST(BL,            "BL",                  "cccc1011vvvvvvvvvvvvvvvvvvvvvvvv") // v1
INST(BX,            "BX",                  "cccc000100101111111111110001mmmm") // v4T
INST(BXJ,           "BXJ",                 "cccc000100101111111111110010mmmm") // v5J

// CRC32 instructions
INST(CRC32,         "CRC32",               "cccc00010zz0nnnndddd00000100mmmm") // v8
INST(CRC32C,        "CRC32C",              "cccc00010zz0nnnndddd00100100mmmm") // v8

// Coprocessor instructions
INST(CDP,           "CDP",                 "cccc1110ooooNNNNDDDDppppooo0MMMM") // v2  (CDP2:  v5)
INST(LDC,           "LDC",                 "cccc110pudw1nnnnDDDDppppvvvvvvvv") // v2  (LDC2:  v5)
INST(MCR,           "MCR",                 "cccc1110ooo0NNNNttttppppooo1MMMM") // v2  (MCR2:  v5)
INST(MCRR,          "MCRR",                "cccc11000100uuuuttttppppooooMMMM") // v5E (MCRR2: v6)
INST(MRC,           "MRC",                 "cccc1110ooo1NNNNttttppppooo1MMMM") // v2  (MRC2:  v5)
INST(MRRC,          "MRRC",                "cccc11000101uuuuttttppppooooMMMM") // v5E (MRRC2: v6)
INST(STC,           "STC",                 "cccc110pudw0nnnnDDDDppppvvvvvvvv") // v2  (STC2:  v5)

// Data Processing instructions
INST(ADC_imm,       "ADC (imm)",           "cccc0010101Snnnnddddrrrrvvvvvvvv") // v1
INST(ADC_reg,       "ADC (reg)",           "cccc0000101Snnnnddddvvvvvrr0mmmm") // v1
INST(ADC_rsr,       "ADC (rsr)",           "cccc0000101Snnnnddddssss0rr1mmmm") // v1
INST(ADD_imm, "ADD (imm)", "cccc0010100Snnnnddddrrrrvvvvvvvv")  // v1
INST(ADD_reg,       "ADD (reg)",           "cccc0000100Snnnnddddvvvvvrr0mmmm") // v1
INST(ADD_rsr,       "ADD (rsr)",           "cccc0000100Snnnnddddssss0rr1mmmm") // v1
INST(AND_imm,       "AND (imm)",           "cccc0010000Snnnnddddrrrrvvvvvvvv") // v1
INST(AND_reg,       "AND (reg)",           "cccc0000000Snnnnddddvvvvvrr0mmmm") // v1
INST(AND_rsr,       "AND (rsr)",           "cccc0000000Snnnnddddssss0rr1mmmm") // v1
INST(BIC_imm,       "BIC (imm)",           "cccc0011110Snnnnddddrrrrvvvvvvvv") // v1
INST(BIC_reg,       "BIC (reg)",           "cccc0001110Snnnnddddvvvvvrr0mmmm") // v1
INST(BIC_rsr,       "BIC (rsr)",           "cccc0001110Snnnnddddssss0rr1mmmm") // v1
INST(CMN_imm,       "CMN (imm)",           "cccc00110111nnnn0000rrrrvvvvvvvv") // v1
INST(CMN_reg,       "CMN (reg)",           "cccc00010111nnnn0000vvvvvrr0mmmm") // v1
INST(CMN_rsr,       "CMN (rsr)",           "cccc00010111nnnn0000ssss0rr1mmmm") // v1
INST(CMP_imm,       "CMP (imm)",           "cccc00110101nnnn0000rrrrvvvvvvvv") // v1
INST(CMP_reg,       "CMP (reg)",           "cccc00010101nnnn0000vvvvvrr0mmmm") // v1
INST(CMP_rsr,       "CMP (rsr)",           "cccc00010101nnnn0000ssss0rr1mmmm") // v1
INST(EOR_imm,       "EOR (imm)",           "cccc0010001Snnnnddddrrrrvvvvvvvv") // v1
INST(EOR_reg,       "EOR (reg)",           "cccc0000001Snnnnddddvvvvvrr0mmmm") // v1
INST(EOR_rsr,       "EOR (rsr)",           "cccc0000001Snnnnddddssss0rr1mmmm") // v1
INST(MOV_imm,       "MOV (imm)",           "cccc0011101S0000ddddrrrrvvvvvvvv") // v1
INST(MOV_reg,       "MOV (reg)",           "cccc0001101S0000ddddvvvvvrr0mmmm") // v1
INST(MOV_rsr,       "MOV (rsr)",           "cccc0001101S0000ddddssss0rr1mmmm") // v1
INST(MVN_imm,       "MVN (imm)",           "cccc0011111S0000ddddrrrrvvvvvvvv") // v1
INST(MVN_reg,       "MVN (reg)",           "cccc0001111S0000ddddvvvvvrr0mmmm") // v1
INST(MVN_rsr,       "MVN (rsr)",           "cccc0001111S0000ddddssss0rr1mmmm") // v1
INST(ORR_imm,       "ORR (imm)",           "cccc0011100Snnnnddddrrrrvvvvvvvv") // v1
INST(ORR_reg,       "ORR (reg)",           "cccc0001100Snnnnddddvvvvvrr0mmmm") // v1
INST(ORR_rsr,       "ORR (rsr)",           "cccc0001100Snnnnddddssss0rr1mmmm") // v1
INST(RSB_imm,       "RSB (imm)",           "cccc0010011Snnnnddddrrrrvvvvvvvv") // v1
INST(RSB_reg,       "RSB (reg)",           "cccc0000011Snnnnddddvvvvvrr0mmmm") // v1
INST(RSB_rsr,       "RSB (rsr)",           "cccc0000011Snnnnddddssss0rr1mmmm") // v1
INST(RSC_imm,       "RSC (imm)",           "cccc0010111Snnnnddddrrrrvvvvvvvv") // v1
INST(RSC_reg,       "RSC (reg)",           "cccc0000111Snnnnddddvvvvvrr0mmmm") // v1
INST(RSC_rsr,       "RSC (rsr)",           "cccc0000111Snnnnddddssss0rr1mmmm") // v1
INST(SBC_imm,       "SBC (imm)",           "cccc0010110Snnnnddddrrrrvvvvvvvv") // v1
INST(SBC_reg,       "SBC (reg)",           "cccc0000110Snnnnddddvvvvvrr0mmmm") // v1
INST(SBC_rsr,       "SBC (rsr)",           "cccc0000110Snnnnddddssss0rr1mmmm") // v1
INST(SUB_imm,       "SUB (imm)",           "cccc0010010Snnnnddddrrrrvvvvvvvv") // v1
INST(SUB_reg,       "SUB (reg)",           "cccc0000010Snnnnddddvvvvvrr0mmmm") // v1
INST(SUB_rsr,       "SUB (rsr)",           "cccc0000010Snnnnddddssss0rr1mmmm") // v1
INST(TEQ_imm,       "TEQ (imm)",           "cccc00110011nnnn0000rrrrvvvvvvvv") // v1
INST(TEQ_reg,       "TEQ (reg)",           "cccc00010011nnnn0000vvvvvrr0mmmm") // v1
INST(TEQ_rsr,       "TEQ (rsr)",           "cccc00010011nnnn0000ssss0rr1mmmm") // v1
INST(TST_imm,       "TST (imm)",           "cccc00110001nnnn0000rrrrvvvvvvvv") // v1
INST(TST_reg,       "TST (reg)",           "cccc00010001nnnn0000vvvvvrr0mmmm") // v1
INST(TST_rsr,       "TST (rsr)",           "cccc00010001nnnn0000ssss0rr1mmmm") // v1

// Exception Generating instructions
INST(BKPT,          "BKPT",                "cccc00010010vvvvvvvvvvvv0111vvvv") // v5
INST(SVC,           "SVC",                 "cccc1111vvvvvvvvvvvvvvvvvvvvvvvv") // v1
INST(UDF,           "UDF",                 "111001111111------------1111----")

// Extension instructions
INST(SXTB,          "SXTB",                "cccc011010101111ddddrr000111mmmm") // v6
INST(SXTB16,        "SXTB16",              "cccc011010001111ddddrr000111mmmm") // v6
INST(SXTH,          "SXTH",                "cccc011010111111ddddrr000111mmmm") // v6
INST(SXTAB,         "SXTAB",               "cccc01101010nnnnddddrr000111mmmm") // v6
INST(SXTAB16,       "SXTAB16",             "cccc01101000nnnnddddrr000111mmmm") // v6
INST(SXTAH,         "SXTAH",               "cccc01101011nnnnddddrr000111mmmm") // v6
INST(UXTB,          "UXTB",                "cccc011011101111ddddrr000111mmmm") // v6
INST(UXTB16,        "UXTB16",              "cccc011011001111ddddrr000111mmmm") // v6
INST(UXTH,          "UXTH",                "cccc011011111111ddddrr000111mmmm") // v6
INST(UXTAB,         "UXTAB",               "cccc01101110nnnnddddrr000111mmmm") // v6
INST(UXTAB16,       "UXTAB16",             "cccc01101100nnnnddddrr000111mmmm") // v6
INST(UXTAH,         "UXTAH",               "cccc01101111nnnnddddrr000111mmmm") // v6

// Hint instructions
INST(PLD_imm,       "PLD (imm)",           "11110101uz01nnnn1111iiiiiiiiiiii") // v5E for PLD; v7 for PLDW
INST(PLD_reg,       "PLD (reg)",           "11110111uz01nnnn1111iiiiitt0mmmm") // v5E for PLD; v7 for PLDW
INST(SEV,           "SEV",                 "----0011001000001111000000000100") // v6K
INST(SEVL,          "SEVL",                "----0011001000001111000000000101") // v8
INST(WFE,           "WFE",                 "----0011001000001111000000000010") // v6K
INST(WFI,           "WFI",                 "----0011001000001111000000000011") // v6K
INST(YIELD,         "YIELD",               "----0011001000001111000000000001") // v6K
INST(NOP,           "Reserved Hint",       "----0011001000001111------------")
INST(NOP,           "Reserved Hint",       "----001100100000111100000000----")

// Synchronization Primitive instructions
INST(CLREX,         "CLREX",               "11110101011111111111000000011111") // v6K
INST(SWP,           "SWP",                 "cccc00010000nnnntttt00001001uuuu") // v2S (v6: Deprecated)
INST(SWPB,          "SWPB",                "cccc00010100nnnntttt00001001uuuu") // v2S (v6: Deprecated)
INST(STL,           "STL",                 "cccc00011000nnnn111111001001tttt") // v8
INST(STLEX,         "STLEX",               "cccc00011000nnnndddd11101001tttt") // v8
INST(STREX,         "STREX",               "cccc00011000nnnndddd11111001mmmm") // v6
INST(LDA,           "LDA",                 "cccc00011001nnnndddd110010011111") // v8
INST(LDAEX,         "LDAEX",               "cccc00011001nnnndddd111010011111") // v8
INST(LDREX,         "LDREX",               "cccc00011001nnnndddd111110011111") // v6
INST(STLEXD,        "STLEXD",              "cccc00011010nnnndddd11101001mmmm") // v8
INST(STREXD,        "STREXD",              "cccc00011010nnnndddd11111001mmmm") // v6K
INST(LDAEXD,        "LDAEXD",              "cccc00011011nnnndddd111010011111") // v8
INST(LDREXD,        "LDREXD",              "cccc00011011nnnndddd111110011111") // v6K
INST(STLB,          "STLB",                "cccc00011100nnnn111111001001tttt") // v8
INST(STLEXB,        "STLEXB",              "cccc00011100nnnndddd11101001mmmm") // v8
INST(STREXB,        "STREXB",              "cccc00011100nnnndddd11111001mmmm") // v6K
INST(LDAB,          "LDAB",                "cccc00011101nnnndddd110010011111") // v8
INST(LDAEXB,        "LDAEXB",              "cccc00011101nnnndddd111010011111") // v8
INST(LDREXB,        "LDREXB",              "cccc00011101nnnndddd111110011111") // v6K
INST(STLH,          "STLH",                "cccc00011110nnnn111111001001mmmm") // v8
INST(STLEXH,        "STLEXH",              "cccc00011110nnnndddd11101001mmmm") // v8
INST(STREXH,        "STREXH",              "cccc00011110nnnndddd11111001mmmm") // v6K
INST(LDAH,          "LDAH",                "cccc00011111nnnndddd110010011111") // v8
INST(LDAEXH,        "LDAEXH",              "cccc00011111nnnndddd111010011111") // v8
INST(LDREXH,        "LDREXH",              "cccc00011111nnnndddd111110011111") // v6K

// Load/Store instructions
INST(LDRBT,         "LDRBT (A1)",          "----0100-111--------------------") // v1
INST(LDRBT,         "LDRBT (A2)",          "----0110-111---------------0----") // v1
INST(LDRHT,         "LDRHT (A1)",          "----0000-111------------1011----") // v6T2
INST(LDRHT,         "LDRHT (A1)",          "----0000-1111111--------1011----") // v6T2
INST(LDRHT,         "LDRHT (A2)",          "----0000-011--------00001011----") // v6T2
INST(LDRSBT,        "LDRSBT (A1)",         "----0000-111------------1101----") // v6T2
INST(LDRSBT,        "LDRSBT (A2)",         "----0000-011--------00001101----") // v6T2
INST(LDRSHT,        "LDRSHT (A1)",         "----0000-111------------1111----") // v6T2
INST(LDRSHT,        "LDRSHT (A2)",         "----0000-011--------00001111----") // v6T2
INST(LDRT,          "LDRT (A1)",           "----0100-011--------------------") // v1
INST(LDRT,          "LDRT (A2)",           "----0110-011---------------0----") // v1
INST(STRBT,         "STRBT (A1)",          "----0100-110--------------------") // v1
INST(STRBT,         "STRBT (A2)",          "----0110-110---------------0----") // v1
INST(STRHT,         "STRHT (A1)",          "----0000-110------------1011----") // v6T2
INST(STRHT,         "STRHT (A2)",          "----0000-010--------00001011----") // v6T2
INST(STRT,          "STRT (A1)",           "----0100-010--------------------") // v1
INST(STRT,          "STRT (A2)",           "----0110-010---------------0----") // v1
INST(LDR_lit,       "LDR (lit)",           "cccc0101u0011111ttttvvvvvvvvvvvv") // v1
INST(LDR_imm,       "LDR (imm)",           "cccc010pu0w1nnnnttttvvvvvvvvvvvv") // v1
INST(LDR_reg,       "LDR (reg)",           "cccc011pu0w1nnnnttttvvvvvrr0mmmm") // v1
INST(LDRB_lit,      "LDRB (lit)",          "cccc0101u1011111ttttvvvvvvvvvvvv") // v1
INST(LDRB_imm,      "LDRB (imm)",          "cccc010pu1w1nnnnttttvvvvvvvvvvvv") // v1
INST(LDRB_reg,      "LDRB (reg)",          "cccc011pu1w1nnnnttttvvvvvrr0mmmm") // v1
INST(LDRD_lit,      "LDRD (lit)",          "cccc0001u1001111ttttvvvv1101vvvv") // v5E
INST(LDRD_imm,      "LDRD (imm)",          "cccc000pu1w0nnnnttttvvvv1101vvvv") // v5E
INST(LDRD_reg,      "LDRD (reg)",          "cccc000pu0w0nnnntttt00001101mmmm") // v5E
INST(LDRH_lit,      "LDRH (lit)",          "cccc000pu1w11111ttttvvvv1011vvvv") // v4
INST(LDRH_imm,      "LDRH (imm)",          "cccc000pu1w1nnnnttttvvvv1011vvvv") // v4
INST(LDRH_reg,      "LDRH (reg)",          "cccc000pu0w1nnnntttt00001011mmmm") // v4
INST(LDRSB_lit,     "LDRSB (lit)",         "cccc0001u1011111ttttvvvv1101vvvv") // v4
INST(LDRSB_imm,     "LDRSB (imm)",         "cccc000pu1w1nnnnttttvvvv1101vvvv") // v4
INST(LDRSB_reg,     "LDRSB (reg)",         "cccc000pu0w1nnnntttt00001101mmmm") // v4
INST(LDRSH_lit,     "LDRSH (lit)",         "cccc0001u1011111ttttvvvv1111vvvv") // v4
INST(LDRSH_imm,     "LDRSH (imm)",         "cccc000pu1w1nnnnttttvvvv1111vvvv") // v4
INST(LDRSH_reg,     "LDRSH (reg)",         "cccc000pu0w1nnnntttt00001111mmmm") // v4
INST(STR_imm,       "STR (imm)",           "cccc010pu0w0nnnnttttvvvvvvvvvvvv") // v1
INST(STR_reg,       "STR (reg)",           "cccc011pu0w0nnnnttttvvvvvrr0mmmm") // v1
INST(STRB_imm,      "STRB (imm)",          "cccc010pu1w0nnnnttttvvvvvvvvvvvv") // v1
INST(STRB_reg,      "STRB (reg)",          "cccc011pu1w0nnnnttttvvvvvrr0mmmm") // v1
INST(STRD_imm,      "STRD (imm)",          "cccc000pu1w0nnnnttttvvvv1111vvvv") // v5E
INST(STRD_reg,      "STRD (reg)",          "cccc000pu0w0nnnntttt00001111mmmm") // v5E
INST(STRH_imm,      "STRH (imm)",          "cccc000pu1w0nnnnttttvvvv1011vvvv") // v4
INST(STRH_reg,      "STRH (reg)",          "cccc000pu0w0nnnntttt00001011mmmm") // v4

// Load/Store Multiple instructions
INST(LDM,           "LDM",                 "cccc100010w1nnnnxxxxxxxxxxxxxxxx") // v1
INST(LDMDA,         "LDMDA",               "cccc100000w1nnnnxxxxxxxxxxxxxxxx") // v1
INST(LDMDB,         "LDMDB",               "cccc100100w1nnnnxxxxxxxxxxxxxxxx") // v1
INST(LDMIB,         "LDMIB",               "cccc100110w1nnnnxxxxxxxxxxxxxxxx") // v1
INST(LDM_usr,       "LDM (usr reg)",       "----100--101--------------------") // v1
INST(LDM_eret,      "LDM (exce ret)",      "----100--1-1----1---------------") // v1
INST(STM,           "STM",                 "cccc100010w0nnnnxxxxxxxxxxxxxxxx") // v1
INST(STMDA,         "STMDA",               "cccc100000w0nnnnxxxxxxxxxxxxxxxx") // v1
INST(STMDB,         "STMDB",               "cccc100100w0nnnnxxxxxxxxxxxxxxxx") // v1
INST(STMIB,         "STMIB",               "cccc100110w0nnnnxxxxxxxxxxxxxxxx") // v1
INST(STM_usr,       "STM (usr reg)",       "----100--100--------------------") // v1

// Miscellaneous instructions
INST(BFC,           "BFC",                 "cccc0111110vvvvvddddvvvvv0011111") // v6T2
INST(BFI,           "BFI",                 "cccc0111110vvvvvddddvvvvv001nnnn") // v6T2
INST(CLZ,           "CLZ",                 "cccc000101101111dddd11110001mmmm") // v5
INST(MOVT,          "MOVT",                "cccc00110100vvvvddddvvvvvvvvvvvv") // v6T2
INST(MOVW,          "MOVW",                "cccc00110000vvvvddddvvvvvvvvvvvv") // v6T2
INST(NOP,           "NOP",                 "----0011001000001111000000000000") // v6K
INST(SBFX,          "SBFX",                "cccc0111101wwwwwddddvvvvv101nnnn") // v6T2
INST(SEL,           "SEL",                 "cccc01101000nnnndddd11111011mmmm") // v6
INST(UBFX,          "UBFX",                "cccc0111111wwwwwddddvvvvv101nnnn") // v6T2

// Unsigned Sum of Absolute Differences instructions
INST(USAD8,         "USAD8",               "cccc01111000dddd1111mmmm0001nnnn") // v6
INST(USADA8,        "USADA8",              "cccc01111000ddddaaaammmm0001nnnn") // v6

// Packing instructions
INST(PKHBT,         "PKHBT",               "cccc01101000nnnnddddvvvvv001mmmm") // v6K
INST(PKHTB,         "PKHTB",               "cccc01101000nnnnddddvvvvv101mmmm") // v6K

// Reversal instructions
INST(RBIT,          "RBIT",                "cccc011011111111dddd11110011mmmm") // v6T2
INST(REV,           "REV",                 "cccc011010111111dddd11110011mmmm") // v6
INST(REV16,         "REV16",               "cccc011010111111dddd11111011mmmm") // v6
INST(REVSH,         "REVSH",               "cccc011011111111dddd11111011mmmm") // v6

// Saturation instructions
INST(SSAT,          "SSAT",                "cccc0110101vvvvvddddvvvvvr01nnnn") // v6
INST(SSAT16,        "SSAT16",              "cccc01101010vvvvdddd11110011nnnn") // v6
INST(USAT,          "USAT",                "cccc0110111vvvvvddddvvvvvr01nnnn") // v6
INST(USAT16,        "USAT16",              "cccc01101110vvvvdddd11110011nnnn") // v6

// Divide instructions
INST(SDIV,          "SDIV",                "cccc01110001dddd1111mmmm0001nnnn") // v7a
INST(UDIV,          "UDIV",                "cccc01110011dddd1111mmmm0001nnnn") // v7a

// Multiply (Normal) instructions
INST(MLA,           "MLA",                 "cccc0000001Sddddaaaammmm1001nnnn") // v2
INST(MLS,           "MLS",                 "cccc00000110ddddaaaammmm1001nnnn") // v6T2
INST(MUL,           "MUL",                 "cccc0000000Sdddd0000mmmm1001nnnn") // v2

// Multiply (Long) instructions
INST(SMLAL,         "SMLAL",               "cccc0000111Sddddaaaammmm1001nnnn") // v3M
INST(SMULL,         "SMULL",               "cccc0000110Sddddaaaammmm1001nnnn") // v3M
INST(UMAAL,         "UMAAL",               "cccc00000100ddddaaaammmm1001nnnn") // v6
INST(UMLAL,         "UMLAL",               "cccc0000101Sddddaaaammmm1001nnnn") // v3M
INST(UMULL,         "UMULL",               "cccc0000100Sddddaaaammmm1001nnnn") // v3M

// Multiply (Halfword) instructions
INST(SMLALxy,       "SMLALXY",             "cccc00010100ddddaaaammmm1xy0nnnn") // v5xP
INST(SMLAxy,        "SMLAXY",              "cccc00010000ddddaaaammmm1xy0nnnn") // v5xP
INST(SMULxy,        "SMULXY",              "cccc00010110dddd0000mmmm1xy0nnnn") // v5xP

// Multiply (Word by Halfword) instructions
INST(SMLAWy,        "SMLAWY",              "cccc00010010ddddaaaammmm1y00nnnn") // v5xP
INST(SMULWy,        "SMULWY",              "cccc00010010dddd0000mmmm1y10nnnn") // v5xP

// Multiply (Most Significant Word) instructions
INST(SMMUL,         "SMMUL",               "cccc01110101dddd1111mmmm00R1nnnn") // v6
INST(SMMLA,         "SMMLA",               "cccc01110101ddddaaaammmm00R1nnnn") // v6
INST(SMMLS,         "SMMLS",               "cccc01110101ddddaaaammmm11R1nnnn") // v6

// Multiply (Dual) instructions
INST(SMLAD,         "SMLAD",               "cccc01110000ddddaaaammmm00M1nnnn") // v6
INST(SMLALD,        "SMLALD",              "cccc01110100ddddaaaammmm00M1nnnn") // v6
INST(SMLSD,         "SMLSD",               "cccc01110000ddddaaaammmm01M1nnnn") // v6
INST(SMLSLD,        "SMLSLD",              "cccc01110100ddddaaaammmm01M1nnnn") // v6
INST(SMUAD,         "SMUAD",               "cccc01110000dddd1111mmmm00M1nnnn") // v6
INST(SMUSD,         "SMUSD",               "cccc01110000dddd1111mmmm01M1nnnn") // v6

// Parallel Add/Subtract (Modulo) instructions
INST(SADD8,         "SADD8",               "cccc01100001nnnndddd11111001mmmm") // v6
INST(SADD16,        "SADD16",              "cccc01100001nnnndddd11110001mmmm") // v6
INST(SASX,          "SASX",                "cccc01100001nnnndddd11110011mmmm") // v6
INST(SSAX,          "SSAX",                "cccc01100001nnnndddd11110101mmmm") // v6
INST(SSUB8,         "SSUB8",               "cccc01100001nnnndddd11111111mmmm") // v6
INST(SSUB16,        "SSUB16",              "cccc01100001nnnndddd11110111mmmm") // v6
INST(UADD8,         "UADD8",               "cccc01100101nnnndddd11111001mmmm") // v6
INST(UADD16,        "UADD16",              "cccc01100101nnnndddd11110001mmmm") // v6
INST(UASX,          "UASX",                "cccc01100101nnnndddd11110011mmmm") // v6
INST(USAX,          "USAX",                "cccc01100101nnnndddd11110101mmmm") // v6
INST(USUB8,         "USUB8",               "cccc01100101nnnndddd11111111mmmm") // v6
INST(USUB16,        "USUB16",              "cccc01100101nnnndddd11110111mmmm") // v6

// Parallel Add/Subtract (Saturating) instructions
INST(QADD8,         "QADD8",               "cccc01100010nnnndddd11111001mmmm") // v6
INST(QADD16,        "QADD16",              "cccc01100010nnnndddd11110001mmmm") // v6
INST(QASX,          "QASX",                "cccc01100010nnnndddd11110011mmmm") // v6
INST(QSAX,          "QSAX",                "cccc01100010nnnndddd11110101mmmm") // v6
INST(QSUB8,         "QSUB8",               "cccc01100010nnnndddd11111111mmmm") // v6
INST(QSUB16,        "QSUB16",              "cccc01100010nnnndddd11110111mmmm") // v6
INST(UQADD8,        "UQADD8",              "cccc01100110nnnndddd11111001mmmm") // v6
INST(UQADD16,       "UQADD16",             "cccc01100110nnnndddd11110001mmmm") // v6
INST(UQASX,         "UQASX",               "cccc01100110nnnndddd11110011mmmm") // v6
INST(UQSAX,         "UQSAX",               "cccc01100110nnnndddd11110101mmmm") // v6
INST(UQSUB8,        "UQSUB8",              "cccc01100110nnnndddd11111111mmmm") // v6
INST(UQSUB16,       "UQSUB16",             "cccc01100110nnnndddd11110111mmmm") // v6

// Parallel Add/Subtract (Halving) instructions
INST(SHADD8,        "SHADD8",              "cccc01100011nnnndddd11111001mmmm") // v6
INST(SHADD16,       "SHADD16",             "cccc01100011nnnndddd11110001mmmm") // v6
INST(SHASX,         "SHASX",               "cccc01100011nnnndddd11110011mmmm") // v6
INST(SHSAX,         "SHSAX",               "cccc01100011nnnndddd11110101mmmm") // v6
INST(SHSUB8,        "SHSUB8",              "cccc01100011nnnndddd11111111mmmm") // v6
INST(SHSUB16,       "SHSUB16",             "cccc01100011nnnndddd11110111mmmm") // v6
INST(UHADD8,        "UHADD8",              "cccc01100111nnnndddd11111001mmmm") // v6
INST(UHADD16,       "UHADD16",             "cccc01100111nnnndddd11110001mmmm") // v6
INST(UHASX,         "UHASX",               "cccc01100111nnnndddd11110011mmmm") // v6
INST(UHSAX,         "UHSAX",               "cccc01100111nnnndddd11110101mmmm") // v6
INST(UHSUB8,        "UHSUB8",              "cccc01100111nnnndddd11111111mmmm") // v6
INST(UHSUB16,       "UHSUB16",             "cccc01100111nnnndddd11110111mmmm") // v6

// Saturated Add/Subtract instructions
INST(QADD,          "QADD",                "cccc00010000nnnndddd00000101mmmm") // v5xP
INST(QSUB,          "QSUB",                "cccc00010010nnnndddd00000101mmmm") // v5xP
INST(QDADD,         "QDADD",               "cccc00010100nnnndddd00000101mmmm") // v5xP
INST(QDSUB,         "QDSUB",               "cccc00010110nnnndddd00000101mmmm") // v5xP

// Status Register Access instructions
INST(CPS,           "CPS",                 "111100010000---00000000---0-----") // v6
INST(SETEND,        "SETEND",              "1111000100000001000000e000000000") // v6
INST(MRS,           "MRS",                 "cccc000100001111dddd000000000000") // v3
INST(MSR_imm,       "MSR (imm)",           "cccc00110010mmmm1111rrrrvvvvvvvv") // v3
INST(MSR_reg,       "MSR (reg)",           "cccc00010010mmmm111100000000nnnn") // v3
INST(RFE,           "RFE",                 "1111100--0-1----0000101000000000") // v6
INST(SRS,           "SRS",                 "1111100--1-0110100000101000-----") // v6
