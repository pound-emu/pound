#ifndef BALLISTIC_BAL_X86_ASSEMBLER_H
#define BALLISTIC_BAL_X86_ASSEMBLER_H

#include "bal_errors.h"
#include "bal_memory.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef enum
    {
        BAL_X86_INVALID = -1,

        /// Accumulator Register.
        BAL_X86_RAX = 0,

        /// Counter Register.
        BAL_X86_RCX,

        /// Data Register.
        BAL_X86_RDX,

        /// Base register.
        BAL_X86_RBX,

        /// Stack pointer.
        BAL_X86_RSP,

        /// Base pointer.
        ///
        /// We use this register holds the base [`bal_cpu_t`] context pointer.
        BAL_X86_RBP,

        /// Source Index.
        BAL_X86_RSI,

        /// Destination Index.
        BAL_X86_RDI,

        /// Extended Register 8.
        BAL_X86_R8,

        /// Extended Register 9.
        BAL_X86_R9,

        /// Extended Register 10.
        BAL_X86_R10,

        /// Extended Register 11.
        BAL_X86_R11,

        /// Extended Register 12.
        BAL_X86_R12,

        /// Extended Register 13.
        BAL_X86_R13,

        /// Extended Register 14.
        BAL_X86_R14,

        /// Extended Register 15.
        BAL_X86_R15,
    } bal_x86_register_t;

    /// x86 Condition Codes for SETcc and Jcc instructions.
    typedef enum
    {
        BAL_X86_COND_O  = 0x0, // Overflow (OF = 1)
        BAL_X86_COND_NO = 0x1, // No Overflow (OF = 0)
        BAL_X86_COND_B  = 0x2, // Below / Carry (CF = 1)
        BAL_X86_COND_AE = 0x3, // Above or Equal / No Carry (CF = 0)
        BAL_X86_COND_E  = 0x4, // Equal / Zero (ZF = 1)
        BAL_X86_COND_NE = 0x5, // Not Equal / Not Zero (ZF = 0)
        BAL_X86_COND_BE = 0x6, // Below or Equal (CF = 1 or ZF = 1)
        BAL_X86_COND_A  = 0x7, // Above (CF = 0 and ZF = 0)
        BAL_X86_COND_S  = 0x8, // Sign (SF = 1)
        BAL_X86_COND_NS = 0x9, // No Sign (SF = 0)
        BAL_X86_COND_P  = 0xA, // Parity (PF = 1)
        BAL_X86_COND_NP = 0xB, // No Parity (PF = 0)
        BAL_X86_COND_L  = 0xC, // Less (SF != OF)
        BAL_X86_COND_GE = 0xD, // Greater or Equal (SF == OF)
        BAL_X86_COND_LE = 0xE, // Less or Equal (ZF = 1 or SF != OF)
        BAL_X86_COND_G  = 0xF, // Greater (ZF = 0 and SF == OF)
    } bal_x86_condition_t;

    static_assert(0xF == BAL_X86_COND_G,
                  "BAL_X86_COND_G Must be the last element in the enum and equals to 0xF to cast "
                  "to uint8_t safely");

    /// The x86-64 assembler state.
    typedef struct
    {
        /// Memory buffer where machine code bytes are written.
        uint8_t *buffer;

        /// Pointer to the executable view of the memory buffer.
        uint8_t *rx_buffer;

        /// Maximum number of bytes this assembler can write.
        size_t capacity;

        /// Current byte index in the buffer.
        size_t offset;

        /// Current error state. If this is not [`BAL_SUCCESS`], all emit calls will fail.
        bal_error_t status;

        uint32_t pad;
    } bal_x86_assembler_t;

    static_assert(40 == sizeof(bal_x86_assembler_t), "Struct size mismatch");

    /// Initializes the x86-64 assembler with `executable_buffer`.
    ///
    /// # Safety
    ///
    /// `executable_buffer` must point to a valid allocation of at least `size` bytes. The buffer
    /// must also be configured with executable page permissions by the host OS.
    ///
    /// # Errors
    ///
    /// Returns [`BAL_SUCCESS`] on success.
    ///
    /// Returns [`BAL_ERROR_INVALID_ARGUMENT`] if `assembler` or `executable_buffer` is `NULL` or
    /// `size` is 0.
    bal_error_t bal_x86_assembler_init(bal_x86_assembler_t    *assembler,
                                       bal_executable_buffer_t executable_buffer,
                                       size_t                  size);

    /// Resets the assembler back to its initial state.
    ///
    /// # Safety
    ///
    /// `assembler` must be valid.
    void bal_x86_assembler_reset(bal_x86_assembler_t *assembler);

    /// Emits an add immediate value to a memory location pointed to by RBP + offset.
    ///
    /// Assembly equivalent: `add qword ptr [rbp + offset], immediate`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_add_mem64_rbp_offset_imm(bal_x86_assembler_t *assembler,
                                               int32_t              offset,
                                               int32_t              immediate);

    /// Emits an add 32-bit immediate value to a 64-bit register.
    ///
    /// Assembly equivalent: `add destination, immediate`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_add_r64_imm32(bal_x86_assembler_t *assembler,
                                    bal_x86_register_t   destination,
                                    int32_t              immediate);

    /// Emits an add 64-bit register to another 64-bit register.
    ///
    /// Assembly equivalent: `add destination, source`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_add_r64_r64(bal_x86_assembler_t *assembler,
                                  bal_x86_register_t   destination,
                                  bal_x86_register_t   source);

    /// Emits a compare byte instruction between a memory location pointed to by RBP + offset and
    /// an 8-bit immediate.
    ///
    /// Assembly equivalent: `cmp byte ptr [rbp + offset], immediate`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_cmp_mem8_rbp_offset_imm(bal_x86_assembler_t *assembler,
                                              int32_t              offset,
                                              int8_t               immediate);

    /// Emits a subtract 32-bit immediate from a 64-bit register.
    ///
    /// Assembly equivalent: `sub destination, immediate`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_sub_r64_imm32(bal_x86_assembler_t     *assembler,
                                    const bal_x86_register_t destination,
                                    const int32_t            immediate);

    /// Emits a subtract 64-bit register from another 64-bit register.
    ///
    /// Assembly equivalent: `sub destination, source`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_sub_r64_r64(bal_x86_assembler_t *assembler,
                                  bal_x86_register_t   destination,
                                  bal_x86_register_t   source);

    /// Emits a bitwise AND instruction between two 64-bit registers.
    ///
    /// Assembly equivalent: `and destination, source`.
    ///
    /// # Safety
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_and_r64_r64(bal_x86_assembler_t *assembler,
                                  bal_x86_register_t   destination,
                                  bal_x86_register_t   source);

    /// Emits a conditional jump to a relative 32-bit offset.
    ///
    /// Assembler equivalent : `jcc offset`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`].
    /// - `assembler->buffer` is full.
    void bal_x86_emit_jcc_rel32(bal_x86_assembler_t *assembler,
                                bal_x86_condition_t  condition,
                                int32_t              offset);

    /// Emits an unconditional jump to the address in the specified 64-bit register.
    ///
    /// Assembly equivalent: `jmp reg64`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`].
    /// - `assembler->buffer` is full.
    void bal_x86_emit_jmp_r64(bal_x86_assembler_t *assembler, bal_x86_register_t reg);

    /// Emits an unconditional jump to a relative 32-bit offset.
    ///
    /// Assembly equivalent: `jmp offset`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_jmp_rel32(bal_x86_assembler_t *assembler, int32_t offset);

    /// Emits a memory load using the 32-bit displacement `offset`.
    ///
    /// Assembler equivalent: `mov destination, [rbp + offset]`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_load_r64_rbp_offset(bal_x86_assembler_t *assembler,
                                          bal_x86_register_t   destination,
                                          int32_t              offset);

    /// Emits an instruction to move a 64-bit immediate value into a 64-bit register.
    ///
    /// Assembly equivalent: `mov destination, immediate`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_mov_r64_imm64(bal_x86_assembler_t *assembler,
                                    bal_x86_register_t   destination,
                                    uint64_t             immediate);

    /// Emits an instruction to move 64-bit register `source` into `destination`.
    ///
    /// Assembly equivalent: `mov reg64, reg64`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_mov_r64_r64(bal_x86_assembler_t *assembler,
                                  bal_x86_register_t   destination,
                                  bal_x86_register_t   source);

    /// Emits a bitwise OR instruction between two 64-bit registers.
    ///
    /// Assembly equivalent: `or destination, source`.
    ///
    /// # Safety
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_or_r64_r64(bal_x86_assembler_t *assembler,
                                 bal_x86_register_t   destination,
                                 bal_x86_register_t   source);

    /// Emits a Stack Pop instruction.
    ///
    /// Assembly equivalent: `push reg64`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_pop_r64(bal_x86_assembler_t *assembler, bal_x86_register_t reg);

    /// Emits a Stack Push instruction.
    ///
    /// Assembly equivalent: `push reg64`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_push_r64(bal_x86_assembler_t *assembler, bal_x86_register_t reg);

    /// Emits a near return instruction.
    ///
    /// Assembly equivalent: `ret`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_ret(bal_x86_assembler_t *assembler);

    /// Emits a SETcc instruction to a memory location pointed to by RBP + offset.
    ///
    /// Assembly equivalent: `setcc byte ptr [rbp + offset]
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_setcc_mem8_rbp_offset(bal_x86_assembler_t *assembler,
                                            bal_x86_condition_t  condition,
                                            int32_t              offset);

    /// Emits a memory store using a 32-bit displacement.
    ///
    /// Assembly equivalent: `mov[rbp + offset], reg64`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_store_r64_rbp_offset(bal_x86_assembler_t *assembler,
                                           bal_x86_register_t   source,
                                           int32_t              offset);

    /// Emit a bitwise TEST instruction between two 64-bit registers.
    ///
    /// Assembly equivalent: `test destination, source`.
    ///
    /// # Safety
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_test_r64_r64(bal_x86_assembler_t *assembler,
                                   bal_x86_register_t   destination,
                                   bal_x86_register_t   source);

    /// Emits a bitwise XOR instruction between two 64-bit registers.
    ///
    /// Assembly equivalent: `xor destination, source`.
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_xor_r64_r64(bal_x86_assembler_t *assembler,
                                  bal_x86_register_t   destination,
                                  bal_x86_register_t   source);

    /// Emits an undefined instruction.
    ///
    /// Assembly equivalent: `ud2`
    ///
    /// # Warning
    ///
    /// This function fails if:
    ///
    /// - `assembler` is `NULL`.
    /// - `assembler->status` != [`BAL_SUCCESS`]
    /// - `assembler->buffer` is full.
    void bal_x86_emit_ud2(bal_x86_assembler_t *assembler);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BALLISTIC_BAL_X86_ASSEMBLER_H

/*** end of file ***/
