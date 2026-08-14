//! This file implements a buffered middleware layer between the Tier1 compiler and the
//! x86 assembler.
//!
//! # Example
//!
//! ```c
//! #include "backend/x86/bal_x86_sliding_window.h"
//! #include "backend/x86/bal_x86_assembler.h"
//! #include "bal_log.h"
//! #include "bal_memory.h"
//! #include <stdint.h>
//!
//! // ---
//! // Allocate memory to represent our executable/writable buffer.
//! uint8_t raw_buffer[1024] = {0};
//! bal_executable_buffer_t exec_buffer = {
//!     .rw_pointer = raw_buffer,
//!     .rx_pointer = raw_buffer,
//! };
//!
//! bal_x86_assembler_t assembler = {0};
//! bal_error_t error = bal_x86_assembler_init(&assembler, exec_buffer, sizeof(raw_buffer));
//! if (error != BAL_SUCCESS) {
//!     return 1;
//! }
//!
//! bal_sliding_window_t window;
//! error = bal_sliding_window_init(&window, &assembler);
//! if (error != BAL_SUCCESS) {
//!     return 1;
//! }
//!
//! // 1. Emit load instruction macro: mov rax, [rbp + 16]
//! bal_x86_macro_t load_macro = {
//!     .opcode = BAL_X86_MACRO_LOAD,
//!     .destination = BAL_X86_RAX,
//!     .source = BAL_X86_RBP,
//!     .immediate_or_offset = 16
//! };
//! bal_sliding_window_push(&window, load_macro);
//!
//! // 2. Emit an identity register move: mov rcx, rcx
//! // The peephole optimizer identifies this and rewrites the opcode to BAL_X86_MACRO_NOP.
//! bal_x86_macro_t identity_macro = {
//!     .opcode = BAL_X86_MACRO_MOV_REGISTER_REGISTER,
//!     .destination = BAL_X86_RCX,
//!     .source = BAL_X86_RCX,
//!     .immediate_or_offset = 0
//! };
//! bal_sliding_window_push(&window, identity_macro);
//!
//! // 3. Lower and write the queued macro items into the assembler buffer.
//! bal_sliding_window_flush_all(&window);
//! ```

#ifndef BALLISTIC_BAL_X86_SLIDING_WINDOW_H
#define BALLISTIC_BAL_X86_SLIDING_WINDOW_H

#include "assert.h"
#include "backend/x86/bal_x86_assembler.h"
#include "bal_assembler.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/// The maximum number of macros held in the sliding window.
#define BAL_SLIDING_WINDOW_CAPACITY 4

    typedef enum
    {
        BAL_X86_MACRO_NOP = 0,

        /// Emits instruction count.
        BAL_X86_MACRO_ADD_CPU_ICOUNT,

        BAL_X86_MACRO_ADD_REGISTER_IMMEDIATE,
        BAL_X86_MACRO_ADD_REGISTER_REGISTER,
        BAL_X86_MACRO_AND_REGISTER_IMMEDIATE,
        BAL_X86_MACRO_JCC_RELATIVE,
        BAL_X86_MACRO_JMP_REGISTER,
        BAL_X86_MACRO_JMP_RELATIVE,
        BAL_X86_MACRO_LOAD,
        BAL_X86_MACRO_MOV_REGISTER_IMMEDIATE,
        BAL_X86_MACRO_MOV_REGISTER_REGISTER,
        BAL_X86_MACRO_OR_REGISTER_IMMEDIATE,
        BAL_X86_MACRO_RET,
        BAL_X86_MACRO_SETCC,
        BAL_X86_MACRO_STORE,
        BAL_X86_MACRO_SUB_REGISTER_IMMEDIATE,
        BAL_X86_MACRO_SUB_REGISTER_REGISTER,
        BAL_X86_MACRO_TEST_REGISTER_REGISTER,
        BAL_X86_MACRO_UD2,
    } bal_x86_macro_opcode_t;

    /// Represents a single un-lowered x86 instruction inside the sliding window.
    typedef struct
    {
        bal_x86_macro_opcode_t opcode;
        bal_x86_register_t     destination;
        bal_x86_register_t     source;
        uint32_t               pad0;
        uint64_t               immediate_or_offset;
        bal_x86_condition_t    condition; // Used for SETcc and Jcc instructions only.
        uint32_t               pad;
    } bal_x86_macro_t;

    static_assert(32 == sizeof(bal_x86_macro_t), "Struct size mismatch");

    /// The Sliding Window Context.
    ///
    /// This struct acts as a middleware between the high level Tier 1 compilers and the
    /// low-level `[bal_x86_assembler_t]`.
    ///
    /// # Warning
    ///
    /// Do not share an instance of this struct across threads to prevent false sharing.
    typedef struct
    {
        /// x86 Assembler context.
        bal_x86_assembler_t *assembler;

        /// The current number queued macros.
        size_t count;

        /// Ring buffer holding the currently queued macros.
        bal_x86_macro_t macros[BAL_SLIDING_WINDOW_CAPACITY];
    } bal_sliding_window_t;

    static_assert(144 == sizeof(bal_sliding_window_t), "Struct size mismatch");

    /// Initializes a new macro sliding window.
    ///
    /// # Safety
    ///
    /// `window` and `assembler must be valid and not `NULL`.
    /// `assembler` must remain valid for the entire lifetime of `window`.
    ///
    /// # Errors
    ///
    /// * Returns [`BAL_SUCCESS`] on success.
    /// * Returns [`BAL_ERROR_INVALID_ARGUMENT`] if any pointer is `NULL`.
    BAL_COLD bal_error_t bal_sliding_window_init(bal_sliding_window_t *window,
                                                 bal_x86_assembler_t  *assembler);

    /// Resets the sliding window back to its initial state.
    ///
    /// # Safety
    ///
    /// `window` must be valid.
    BAL_HOT void bal_sliding_window_reset(bal_sliding_window_t *window);

    /// Pushes a new macro into the sliding window and triggers the peephole optimizer.
    ///
    /// # Safety
    ///
    /// `window` pointer must be initialized via [`bal_sliding_window_init`].
    BAL_HOT void bal_sliding_window_push(bal_sliding_window_t *window, bal_x86_macro_t macro);

    /// Flushes all remaining macros in the window to the executable buffer in `window->assembler`.
    ///
    /// This must
    BAL_HOT void bal_sliding_window_flush_all(bal_sliding_window_t *window);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BALLISTIC_BAL_X86_SLIDING_WINDOW_H

/***  end of file ***/
