//! This module provides a low-level interface for generating ARM64 instructions into a
//! pre-allocated memory buffer.
//!
//! # Examples
//!
//! ```c
//! #include "bal_log.h"
//! #include "bal_assembler.h"
//!
//! uint32_t code[128];
//! bal_assembler_t assembler;
//! bal_logger_init_default();
//! bal_error_t error = bal_assembler_init(&assembler, code, 64);
//! if (error == BAL_SUCCESS)
//! {
//!     // MOV X0, #42
//!     bal_emit_movz(&assembler, BAL_REGISTER_X0, 42, 0);
//! }
//! ```

#ifndef BALLISTIC_ASSEMBLER_H
#define BALLISTIC_ASSEMBLER_H

#include "bal_errors.h"
#include "bal_log.h"
#include "bal_types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /// This struct manages a linear buffer of 32-bit integers where ARM64 machine code is written.
    /// It tracks the current write position and handles boundary checking.
    ///
    /// # Ownership
    ///
    /// The struct does not own the memory backing the `buffer` pointer. The caller retains full
    /// ownership and is responsible for:
    ///
    /// 1. Ensuring the memory pointed to by `buffer` outlives the `bal_assembler_t` instance.
    /// 2. Freeing the underlying memory allocation when the JIT buffer is no longer needed.
    typedef struct
    {
        /// A pointer to the start of the code buffer.
        uint32_t *buffer;

        /// The maximum number of instructions that can fit in the buffer.
        size_t capacity;

        /// The current write index within the buffer.
        size_t offset;

        /// The current error state of the assembler.
        ///
        /// Once this is set to anything other than [`BAL_SUCCESS`], all subsequent emit calls will
        /// be ignored until the assembler is reset.
        bal_error_t status;

        /// Integrity check.
        uint32_t magic;
    } bal_assembler_t;

    /// Initializes the assembler with a specific memory buffer and the size of the buffer in
    /// `uint32_t` elements.
    ///
    /// # Returns
    ///
    /// * [`BAL_SUCCESS`] on success.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `assembler` or `buffer` is NULL, or if `size` is `0`.
    /// * [`BAL_ERROR_MEMORY_ALIGNMENT`] if `buffer` is not 4-byte aligned.
    /// * [`BAL_ERROR_CAPACITY_TOO_BIG`] if `size` is large enough to cause a `size_t` integer
    ///   overflow.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_error_t error = bal_assembler_init(&assembler, code, 64);
    /// assert(error == BAL_SUCCESS);
    /// assert(assembler.capacity == 64);
    /// assert(assembler.offset == 0);
    /// ```
    bal_error_t bal_assembler_init(bal_assembler_t *assembler, void *buffer, size_t size);

    /// Resets the assembler back to its initial state.
    ///
    /// # Errors
    ///
    /// `assembler->status` is set to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `assembler->buffer` is NULL or `assembler->capacity` is
    ///   0.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Safety
    ///
    /// * `assembler` must be a valid pointer.
    /// * `assembler` struct integrity must be valid.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    /// bal_emit_movz(&assembler,BAL_REGISTER_X0, 42, 0);
    /// assert(assembler.offset == 1);
    ///
    /// bal_assembler_reset(&assembler);
    /// assert(assembler.offset == 0);
    /// assert(assembler.status == BAL_SUCCESS);
    /// assert(code[0] == 0);
    /// ```
    void bal_assembler_reset(bal_assembler_t *assembler);

    /// Destroys the assembler context.
    ///
    /// # Safety
    ///
    /// * `assembler` must be a valid pointer.
    /// * This function does NOT free the memory backing the `buffer` pointer.
    /// * Calls to any function that uses this context will fail.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    /// bal_assembler_destroy(&assembler);
    ///
    /// // Emitting after a destroy is a no-op. You must call bal_assembler_init() again.
    /// bal_emit_movz(&assembler, BAL_REGISTER_X0, 42, 0);
    /// assert(assembler.status == BAL_ERROR_STRUCT_CORRUPTED);
    /// ```
    void bal_assembler_destroy(bal_assembler_t *assembler);

    /// Emit a `ADD` (Immediate) instruction.
    ///
    /// Adds a register value `rn` and 12 bit immediate value `imm12` with the optional left shift
    /// `sh` to apply to the immediate, and writes the result to the destination register `rd`.
    ///
    /// # Safety
    ///
    /// * `rd` and `rn` must be valid registers (0-31).
    /// * `imm12` must not exceed 12 bits (0-4095).
    /// * `shift` must be the bit value `0` (LSL #0) or `1` (LSL #12).
    /// * `assembler->buffer` must not be NULL.
    /// * Function does not emit instructions if `assembler->status != BALL_SUCCESS`.
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if function arguments are invalid.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_add_immediate(&assembler, BAL_REGISTER_X0, BAL_REGISTER_X1, 42, 0);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0x9100A820);
    /// ```
    void bal_emit_add_immediate(bal_assembler_t     *assembler,
                                bal_register_index_t rd,
                                bal_register_index_t rn,
                                uint16_t             imm12,
                                uint8_t              shift);

    /// Emits a `ADD` (Shifted Register) instruction.
    ///
    /// Adds a register value `rn` and a shifted register `rm`, and writes the result to `rd`.
    ///
    /// # Safety
    ///
    /// * `rd`, `rn`, `rm` must be valid registers (0-31).
    /// * `shift` must be between 0 and 63.
    /// * `shift_type` must be 0 (LSL), 1 (LSR), or 2 (ASR).
    /// * Function does not emit instructions if `assembler->status` != [`BAL_SUCCESS`].
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `assembler` or `assembler->buffer` is NULL, or if
    ///   register/shift arguments are invalid.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_add_shifted_register(&assembler, BAL_REGISTER_X0, BAL_REGISTER_X1, BAL_REGISTER_X2,
    ///                               0, 0);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0x8B020020);
    /// ```
    void bal_emit_add_shifted_register(bal_assembler_t     *assembler,
                                       bal_register_index_t rd,
                                       bal_register_index_t rn,
                                       bal_register_index_t rm,
                                       uint8_t              shift,
                                       uint8_t              shift_type);

    /// Emits a `B` (Branch) instruction.
    ///
    /// Branches unconditionally to a PC-relative offset.
    ///
    /// # Safety
    ///
    /// * `offset` must be strictly 4-byte aligned.
    /// * `offset` will be truncated if it exceeds 26 bits.
    /// *  Does not emit if `assembler->status` != [`BAL_SUCCESS`]
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `assembler` or `assembler->buffer` is NULL.
    /// * [`BAL_ERROR_PC_ALIGNMENT`] if `offset` is not a multiple of 4.
    /// * [`BAL_ERROR_BRANCH_OFFSET_OVERFLOW`] if `offset` exceeds the signed 26-bit displacement
    ///   limits.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// //---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_b(&assembler, 16);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0x14000004);
    /// ```
    void bal_emit_b(bal_assembler_t *assembler, int32_t offset);

    /// Emits a `BR` (Branch Register) instruction.
    ///
    /// Branches unconditionally to an address in a register.
    ///
    /// # Safety
    ///
    /// * `rn` must be between 0-31 inclusive.
    /// * Does not emit if `assembler->status` != [`BAL_SUCCESS`]
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `assembler` or `assembler->buffer` is NULL or `rn` is
    ///   not between 0-31 inclusive.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_br(&assembler, BAL_REGISTER_X30);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0xD61F03C0);
    /// ```
    void bal_emit_br(bal_assembler_t *assembler, bal_register_index_t rn);

    /// Emit a `SUB` (Immediate) instruction.
    ///
    /// Subtracts a 12 bit immediate value `imm12` with the optional left shift `sh` from register
    /// value `rn`, and writes the result to the destination register `rd`.
    ///
    /// # Safety
    ///
    /// * `rd` must be a valid register index (0-31).
    /// * `rn` must be a valid register index (0-31).
    /// * `imm12` must not exceed 12 bits (maximum 4095).
    /// * `shift` must be exactly `0` (LSL #0) or `1` (LSL #12).
    /// * Function does not emit instructions if `assembler->status != BAL_SUCCESS`.
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `rd` > 31, `rn` > 31, `imm12` > 4095, or `shift` > 1.
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_sub_immediate(&assembler, BAL_REGISTER_X0, BAL_REGISTER_X1, 10, 0);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0xD1002820);
    /// ```
    void bal_emit_sub_immediate(bal_assembler_t     *assembler,
                                bal_register_index_t rd,
                                bal_register_index_t rn,
                                uint16_t             imm12,
                                uint8_t              shift);

    /// Emit a `MOVZ` (Move Wide with Zero) instruction.
    ///
    /// Moves a 16-bit immediate into a register, shifting it left by 0, 16, 32, or 48 bits, and
    /// the rest of the register to zero.
    ///
    /// # Safety
    ///
    /// * `rd` must be a valid register index (0-31).
    /// * `shift` must be 0, 16, 32, or 48.
    /// * Does not emit if `assembler->status != BAL_SUCCESS`.
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if function arguments are invalid.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_movz(&assembler, BAL_REGISTER_X0, 42, 0);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0xD2800540);
    /// ```
    void bal_emit_movz(bal_assembler_t     *assembler,
                       bal_register_index_t rd,
                       uint16_t             imm,
                       uint8_t              shift);

    /// Emits a `MOVK` (Move Wide with Keep) instruction.
    ///
    /// Moves a 16-bit immediate into a specific 16-bit field of a register, leaving the other bits
    /// unchanged.
    ///
    /// # Safety
    ///
    /// * `rd` must be a valid register index (0-31).
    /// * `shift` must be 0, 16, 32, or 48.
    /// * Does not emit if `assembler->status != BAL_SUCCESS`.
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if function arguments are invalid.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    /// bal_emit_movk(&assembler, BAL_REGISTER_X0, 0x1234, 16);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0xF2A24680);
    /// ```
    void bal_emit_movk(bal_assembler_t     *assembler,
                       bal_register_index_t rd,
                       uint16_t             imm,
                       uint8_t              shift);

    /// Emits a `MOVN` (Move Wide with Not) instruction.
    ///
    /// Moves the bitwise inverse of a 16-bit immediate (shifted left) into a register, setting all
    /// other bits to 1.
    ///
    /// # Safety
    ///
    /// * `rd` must be a valid register index (0-31).
    /// * `shift` must be 0, 16, 32, or 48.
    /// * Does not emit if `assembler->status != BAL_SUCCESS`.
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if function arguments are invalid.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_movn(&assembler, BAL_REGISTER_X0, 0xFFFF, 0);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0x929FFFE0);
    /// ```
    void bal_emit_movn(bal_assembler_t     *assembler,
                       bal_register_index_t rd,
                       uint16_t             imm,
                       uint8_t              shift);

    /// Emits a `RET` instruction.
    ///
    /// Returns from subroutine branches unconditionally to an address in a register with a hint
    /// that this is a subroutine return.
    ///
    /// # Safety
    ///
    /// * `rn` must be between 0-31 inclusive.
    /// * Does not emit if `assembler->status != BAL_SUCCESS`.
    ///
    /// # Errors
    ///
    /// Modifies `assembler->status` to the following if an error occurs:
    ///
    /// * [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if `assembler->offset >= assembler->capacity`.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if function arguments are invalid.
    /// * [`BAL_ERROR_STRUCT_CORRUPTED`] if the assembler's magic number fails integrity checks.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_assembler.h"
    /// #include <assert.h>
    ///
    /// // ---
    /// uint32_t code[64];
    /// bal_assembler_t assembler;
    /// bal_logger_init_default();
    /// bal_assembler_init(&assembler, code, 64);
    ///
    /// bal_emit_ret(&assembler, BAL_REGISTER_X30);
    /// assert(assembler.offset == 1);
    /// assert(code[0] == 0xD65F03C0);
    /// ```
    void bal_emit_ret(bal_assembler_t *assembler, bal_register_index_t rn);

#ifdef BALLISTIC_BUILD_TESTS

    void emit_mov(bal_assembler_t *BAL_RESTRICT assembler,
                  const char *BAL_RESTRICT      mnemonic,
                  bal_register_index_t          rd,
                  uint16_t                      imm,
                  uint8_t                       shift,
                  uint32_t                      opcode);

#endif // BALLISTIC_BUILD_TESTS

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BALLISTIC_ASSEMBLER_H */

/*** end of file ***/
