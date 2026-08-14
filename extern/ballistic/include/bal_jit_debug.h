//! This module provides JIT crash debugging and Guest PC tracking for Ballistic.
//!
//! When the host CPU encounters a SIGSEGV or SIGILL inside the JIT buffer, this module
//! intercepts the OS signal, maps the faulting host instruction pointer (RIP) back to the original
//! Guest Program Counter, and invokes a user-defined callback or logs the error before aborting.
//!
//! # Examples
//!
//! Initialization and tracking a JIT block:
//!
//! ```c
//! #include "bal_jit_debug.h"
//! #include "bal_memory.h"
//! #include <assert.h>
//!
//! bal_allocator_t allocator = {0};
//! bal_allocator_default_init(&allocator);
//!
//! bal_logger_init_default();
//!
//! bal_jit_debug_context_t debug_context = {0};
//! bal_error_t err = bal_jit_debug_init(&allocator, &debug_context);
//! assert(err == BAL_SUCCESS);
//!
//! uint8_t jit_code[64] = {0};
//! bal_jit_instruction_map_t mappings[] = {
//!     { .x86_offset = 0,  .guest_pc_offset = 0 },
//!     { .x86_offset = 16, .guest_pc_offset = 4 }
//! };
//!
//! err = bal_jit_debug_add_block(&debug_context, jit_code, sizeof(jit_code), 0x1000, mappings, 2);
//! assert(err == BAL_SUCCESS);
//!
//! err = bal_jit_debug_register_signal_handler(&debug_context, jit_code, sizeof(jit_code));
//! assert(err == BAL_SUCCESS);
//!
//! bal_jit_debug_unregister_signal_handler(&debug_context);
//! bal_jit_debug_destroy(&allocator, &debug_context);
//! ```

#ifndef BALLISTIC_BAL_JIT_DEBUG_H
#define BALLISTIC_BAL_JIT_DEBUG_H

#include "bal_attributes.h"
#include "bal_errors.h"
#include "bal_log.h"
#include "bal_memory.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BAL_JIT_DEBUG_ENTRY_CAPACITY       8192
#define BAL_JIT_DEBUG_ARENA_CAPACITY_BYTES (4 * 1024 * 1024) // 4 MiB

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

    /// Callback when a crash occurs inside a JIT block.
    typedef void (*bal_jit_crash_callback_t)(void       *user_data,
                                             uint64_t    guest_pc,
                                             uint64_t    host_rip,
                                             uint32_t    jit_offset,
                                             const void *jit_block_start,
                                             uint32_t    jit_block_size);

    typedef struct
    {
        /// Byte offset from the start of the JIT block.
        uint32_t x86_offset;

        /// Offset from the JIT block's base Guest PC.
        uint32_t guest_pc_offset;
    } bal_jit_instruction_map_t;

    static_assert(8 == sizeof(bal_jit_instruction_map_t), "Struct size mismatch");

    /// Metadata for a single compiled JIT block.
    typedef struct
    {
        uint64_t                   base_guest_pc;
        uint32_t                   instruction_count;
        uint32_t                   pad0;
        bal_jit_instruction_map_t *mappings;
        uint64_t                   pad1;
    } bal_jit_block_metadata_t;

    static_assert(32 == sizeof(bal_jit_block_metadata_t), "Struct size mismatch");

    typedef struct
    {
        void                     *rx_start;
        uint32_t                  rx_size;
        uint32_t                  pad0;
        bal_jit_block_metadata_t *metadata;
        uint64_t                  pad1;
    } bal_jit_block_entry_t;

    static_assert(32 == sizeof(bal_jit_block_entry_t), "Struct size mismatch");

    BAL_ALIGNED(64) typedef struct
    {
        bal_jit_block_entry_t *entries;

        /// Memory Layout of the arena:
        ///
        /// Offset 0x0000:          bal_jit_block_metadata_t (Block 0).
        /// Offset 0x0020:          Array of bal_jit_instruction_map_t (N * 8 bytes).
        /// Offset 0x0020 + (N*8):  bal_jit_block_metadata_t (Block 1).
        /// Offset ....:            Array of bal_jit_instruction_map_t (M * 8 bytes).
        /// etc...
        uint8_t *metadata_arena;
        size_t   entry_count;

        // The entry list capacity. This is NOT in bytes.
        size_t entry_capacity;
        size_t arena_offset;

        // Cold data.

        // The metadata arena capacity in bytes.
        size_t arena_capacity;

        void                    *jit_buffer_start;
        void                    *jit_buffer_end;
        bal_jit_crash_callback_t crash_callback;
        void                    *crash_callback_user_data;
        bal_error_t              status;

        /// Integrity check.
        uint32_t magic;

        uint8_t pad[40];
    } bal_jit_debug_context_t;

    static_assert(128 == sizeof(bal_jit_debug_context_t), "Struct size mismatch");

    /// Initializes the JIT debug context.
    ///
    /// # Returns
    ///
    /// * [`BAL_SUCCESS`] on success.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if `allocator` or `context` is `NULL`.
    /// * [`BAL_ERROR_ALLOCATION_FAILED`] if the allocator fails to allocate the entries or arena.
    ///
    /// # Examples
    ///
    /// ```c
    //! #include "bal_jit_debug.h"
    //! #include <assert.h>
    //!
    //! bal_allocator_t allocator = {0};
    //! bal_allocator_default_init(&allocator);
    //!
    //! bal_logger_init_default();
    //!
    //! bal_jit_debug_context_t debug_context = {0};
    //! bal_error_t err = bal_jit_debug_init(&allocator, &debug_context);
    //! assert(err == BAL_SUCCESS);
    //!
    //! bal_jit_debug_unregister_signal_handler(&debug_context);
    //! bal_jit_debug_destroy(&allocator, &debug_context);
    /// ```
    BAL_COLD bal_error_t bal_jit_debug_init(const bal_allocator_t   *allocator,
                                            bal_jit_debug_context_t *context);

    /// Destroys the JIT debug context and frees all internal allocations.
    ///
    /// # Safety
    ///
    /// * `allocator` must be a valid pointer to the allocator used during initialization.
    /// * `context` must be a valid pointer to an initialized context.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_jit_debug.h"
    /// #include <assert.h>
    ///
    /// bal_allocator_t allocator = {0};
    /// bal_allocator_default_init(&allocator);
    ///
    /// bal_logger_init_default();
    ///
    /// bal_jit_debug_context_t context = {0};
    /// bal_jit_debug_init(&allocator, &context);
    ///
    /// bal_jit_debug_destroy(&allocator, &context);
    /// assert(context.entries == NULL);
    /// assert(context.metadata_arena == NULL);
    /// ```
    BAL_COLD void bal_jit_debug_destroy(const bal_allocator_t   *allocator,
                                        bal_jit_debug_context_t *context);

    /// Records a newly compiled JIT block and its instruction mappings into the debug context.
    ///
    /// # Safety
    ///
    /// * `context` must be initialized.
    /// * `mapping` must point to an array of at least `instruction_count` valid mappings.
    ///
    /// # Returns
    ///
    /// * [`BAL_SUCCESS`] on success.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if the function arguments are NULL or  if
    ///   `instruction_count` is `0`.
    /// * [`BAL_ERROR_BUFFER_OVERFLOW`] if the entry list or metadata arena is full.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_jit_debug.h"
    /// #include <assert.h>
    ///
    /// bal_allocator_t allocator = {0};
    /// bal_allocator_default_init(&allocator);
    ///
    /// bal_logger_init_default();
    ///
    /// bal_jit_debug_context_t context = {0};
    /// bal_jit_debug_init(&allocator, &context);
    ///
    /// uint8_t jit_code[64] = {0};
    /// bal_jit_instruction_map_t mappings[] = {
    ///     { .x86_offset = 0,  .guest_pc_offset = 0 },
    ///     { .x86_offset = 16, .guest_pc_offset = 4 }
    /// };
    ///
    /// bal_error_t err = bal_jit_debug_add_block(&context, jit_code, sizeof(jit_code), 0x1000,
    ///                                           mappings, 2);
    /// assert(err == BAL_SUCCESS);
    /// assert(context.entry_count == 1);
    ///
    /// bal_jit_debug_destroy(&allocator, &context);
    /// ```
    BAL_HOT bal_error_t bal_jit_debug_add_block(bal_jit_debug_context_t         *context,
                                                void                            *rx_start,
                                                uint32_t                         rx_size,
                                                uint64_t                         base_guest_pc,
                                                const bal_jit_instruction_map_t *mapping,
                                                uint32_t                         instruction_count);

    /// Registers the OS-level signal/exception handler to intercept faults inside the JIT buffer.
    ///
    /// If `context.crash_callback` is not NULL, Ballistic will call it when the compiled JIT code
    /// creates a segmentation fault, else Ballistic logs the fault, crash the program, and
    /// generate a core dump.
    ///
    /// # Safety
    ///
    /// * `jit_buffer_start` must point to a valid executable memory region of at least
    /// `jit_buffer_size` bytes.
    ///
    /// # Returns
    ///
    /// * [`BAL_SUCCESS`] on success.
    /// * [`BAL_ERROR_INVALID_ARGUMENT`] if the function arguments are NULL or if
    ///   `jit_buffer_size` is `0`.
    /// * [`BAL_ERROR_ALLOCATION_FAILED`] if the OS fails to register the handler.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_jit_debug.h"
    /// #include <assert.h>
    ///
    /// bal_allocator_t allocator = {0};
    /// bal_allocator_default_init(&allocator);
    ///
    /// bal_logger_init_default();
    ///
    /// bal_jit_debug_context_t context = {0};
    /// bal_jit_debug_init(&allocator, &context);
    ///
    /// uint8_t jit_buffer[4096] = {0};
    /// bal_error_t err = bal_jit_debug_register_signal_handler(&context, jit_buffer,
    ///                                                         sizeof(jit_buffer));
    /// assert(err == BAL_SUCCESS);
    /// assert(context.jit_buffer_start == jit_buffer);
    ///
    /// bal_jit_debug_unregister_signal_handler(&context);
    /// bal_jit_debug_destroy(&allocator, &context);
    /// ```
    BAL_COLD bal_error_t bal_jit_debug_register_signal_handler(bal_jit_debug_context_t *context,
                                                               void  *jit_buffer_start,
                                                               size_t jit_buffer_size);

    /// Unregisters the OS-level signal/exception handler.
    ///
    /// # Safety
    ///
    /// * `context` must be a valid pointer.
    ///
    /// # Warning
    ///
    /// Calling this function sets `context.jit_buffer_start` and `context.jit_buffer_end` to NULL.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_jit_debug.h"
    /// #include <assert.h>
    ///
    /// bal_allocator_t allocator = {0};
    /// bal_allocator_default_init(&allocator);
    ///
    /// bal_logger_init_default();
    ///
    /// bal_jit_debug_context_t context = {0};
    /// bal_jit_debug_init(&allocator, &context);
    ///
    /// uint8_t jit_buffer[4096] = {0};
    /// bal_jit_debug_register_signal_handler(&context, jit_buffer, sizeof(jit_buffer));
    ///
    /// bal_jit_debug_unregister_signal_handler(&context);
    /// assert(context.jit_buffer_start == NULL);
    /// assert(context.jit_buffer_end == NULL);
    ///
    /// bal_jit_debug_destroy(&allocator, &context);
    /// ```
    BAL_COLD void bal_jit_debug_unregister_signal_handler(bal_jit_debug_context_t *context);

#ifdef BALLISTIC_BUILD_TESTS

    /// This functions is exposed for testing purposes only.
    bool handle_jit_fault(uint64_t rip, uint64_t rbp);

#endif // BALLISTIC_BUILD_TESTS

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BALLISTIC_BAL_JIT_DEBUG_H
