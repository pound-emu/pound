//! The core execution engine for the Ballistic JIT compiler. This is the primary entrypoint for
//! embedding Ballistic into your application.
//!
//! # Architecture
//!
//! The engine relies on three core components provided by the host application:
//! - [`bal_cpu_t`]: Holds the state of the emulated ARM64 guest CPU.
//! - [`bal_allocator_t`]: Provides memory allocation.
//! - [`bal_memory_interface_t`]: Translates Guest Virtual Addresses to Host Virtual Addresses.
//!
//! # Engine Flags
//!
//! You can control the engine's behaviour via the `engine.flags` bitmask. Flags can be set at
//! any time during compilation. See [`bal_engine_flags.h`] for more details.
//!
//! # Examples
//!
//! Basic initialization and execution:
//!
//! ```c
//! #include "bal_engine.h"
//! #include "bal_memory.h"
//! #include "bal_log.h"
//! #include <stdlib.h>
//! #include <string.h>
//!
//! // Setup logging and default system allocator.
//! bal_logger_t logger = {0};
//! bal_logger_init_default(&logger);
//!
//! bal_allocator_t allocator = {0};
//! bal_allocator_default_init(&allocator);
//!
//! // Allocate guest memory and setup a flat 1:1 translation interface,
//! size_t guest_memory_size = 4096;
//! uint32_t *guest_memory = allocator.allocate(allocator.context, 16, guest_memory_size);
//! memset(guest_memory, 0, guest_memory_size);
//!
//! bal_memory_interface_t memory_interface = {0};
//! bal_flat_translation_interface_init(&allocator, &memory_interface, guest_memory,
//! guest_memory_size, logger);
//!
//! // Initialize CPU and Engine.
//! bal_cpu_t cpu = {0};
//! bal_engine_t engine = {0};
//! bal_engine_init(&engine, &cpu, &allocator, &memory_interface, logger);
//!
//! guest_memory[0] = 0xD2800054; // MOVZ X0, #42
//! guest_memory[1] = 0xD65F03C0; // RET
//!
//! cpu.pc = 0;
//! cpu.x[30] = BAL_ENGINE_SENTINEL; // Triggers a safe exit when RET is executed.
//!
//! // Execute the JIT compiled block.
//! bal_engine_run_thread(&engine);
//!
//! // cpu.x[0] now contains 42.
//!
//! // Cleanup.
//! bal_engine_destroy(&engine);
//! bal_flat_translation_interface_destroy(&allocator, &memory_interface);
//! allocator.free(allocator.context, guest_memory, guest_memory_size);
//! ```
//!
//! # Thread Safety
//!
//! [`bal_engine_run_thread`] is designed to run on a single dedicated thread. Functions that
//! control the thread like [`bal_engine_stop_thread`] and [`bal_engine_clear_cache`] are thread
//! safe and can be called from external threads.

#ifndef BALLISTIC_ENGINE_H
#define BALLISTIC_ENGINE_H

#include "backend/bal_cpu.h"
#include "bal_attributes.h"
#include "bal_errors.h"
#include "bal_log.h"
#include "bal_memory.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/// A byte pattern written to memory during initialization, poisoning allocated
/// regions. This is mainly used for detecting reads from uninitialized memory.
#define BAL_POISON_UNINITIALIZED_MEMORY 0xFF

/// Ballistic will safely terminate execution if the Program Counter matches this value.
#define BAL_ENGINE_SENTINEL 0xFFFFFFFFFFFFFFFFULL

    BAL_ALIGNED(64) typedef struct
    {
        /// The guest CPU state.
        bal_cpu_t *cpu;

        /// The allocator used for all internal engine memory.
        const bal_allocator_t *allocator;

        /// The memory interface for all guest-to-host address translation.
        const bal_memory_interface_t *memory_interface;

        /// Opaque pointer to the internal engine's state.
        void *engine_state;

        /// Handles logging for this engine.
        bal_logger_t logger;

        /// The current error state of the engine.
        ///
        /// If an operation fails, this field is set to a specific error code.
        /// See [`bal_error_t`]. Once set to an error state, subsequent operation
        /// on this engine will silently fail until [`bal_engine_reset`] is called.
        bal_error_t status;

        /// Execution flags (e.g., `BAL_ENGINE_FLAG_RUNNING`).
        uint32_t flags;
    } bal_engine_t;

    static_assert(sizeof(bal_engine_t) <= 64, "Engine must fit in a L1 Cache line");

    /// Initializes a Ballistic engine.
    ///
    /// # Safety
    ///
    /// * `engine`, `cpu`, `allocator`, and `memory_interface` must be valid pointers.
    /// * The host application retains ownership of `cpu`, `allocator`, and `memory_interface`.
    ///   They must outlive the `bal_engine_t` instance and remain unmodified while the engine is
    ///   running.
    ///
    /// # Errors
    ///
    /// * Returns [`BAL_SUCCESS`] if the engine is ready for use.
    /// * Returns [`BAL_ERROR_INVALID_ARGUMENT`] if the pointers are `NULL`.
    /// * Returns [`BAL_ERROR_ALLOCATION_FAILED`] if the allocator cannot allocate a memory buffer.
    BAL_COLD bal_error_t bal_engine_init(bal_engine_t                 *engine,
                                         bal_cpu_t                    *cpu,
                                         const bal_allocator_t        *allocator,
                                         const bal_memory_interface_t *memory_interface,
                                         bal_logger_t                  logger);

    /// The sole entry point for executing guest code.
    ///
    /// # Safety
    ///
    /// * This function must run on a dedicated thread.
    /// * Do not call this function concurrently on the same `engine` instance.
    /// * The guest memory mapped via `memory_interface` must remain valid and accessible for the
    ///   duration of the execution.
    ///
    /// # Errors
    ///
    /// * Returns [`BAL_SUCCESS`] when execution halts normally.
    /// * Returns [`BAL_ERROR_ENGINE_ALREADY_RUNNING`] if the thread is still running.
    /// * Returns [`BAL_ERROR_MEMORY_FAULT`] if the guest attempts to fetch instructions from an
    ///   unmapped or invalid Guest Virtual Address (GVA).
    /// * Returns [`BAL_ERROR_PC_ALIGNMENT`] if the guest PC is not 4-byte aligned
    ///   (when `BAL_ENGINE_FLAG_STRICT_ALIGNMENT` is set) or if instruction fetching crosses a page
    ///   boundary improperly.
    /// * Returns [`BAL_ERROR_UNKNOWN_INSTRUCTION`] if the decoder encounters an undefined, invalid,
    ///   or unsupported ARM64 instruction.
    /// * Returns [`BAL_ERROR_INSTRUCTION_OVERFLOW`] if the JIT executable buffer is completely
    ///   exhausted and the block cannot be compiled even after a cache reset.
    /// * Returns [`BAL_ERROR_BRANCH_OFFSET_OVERFLOW`] if a compiled relative branch exceeds the x86
    ///   displacement limits during code generation.
    /// * Returns [`BAL_ERROR_INVALID_ARGUMENT`] if an internal compiler state corruption occurs.
    BAL_HOT bal_error_t bal_engine_run_thread(bal_engine_t *engine);

    /// Stops the engine execution asynchronously.
    ///
    /// This function is thread-safe and can be called from any thread.
    BAL_HOT void bal_engine_stop_thread(bal_engine_t *engine);

    /// Resets `engine` for the next compilation unit. This is a low-cost memory
    /// operation designed to be called between translation units.
    ///
    /// # Safety
    ///
    /// * Must NOT be called while the engine is actively compiling code via
    ///  [`bal_engine_run_thread`].
    /// * `engine` must be a valid pointer.
    ///
    /// # Errors
    ///
    /// * Returns [`BAL_SUCCESS`] on success.
    /// * Returns [`BAL_ERROR_INVALID_ARGUMENT`] if `engine` is `NULl`.
    BAL_HOT bal_error_t bal_engine_reset(bal_engine_t *engine);

    /// Destroys a Ballistic engine and frees all internal resources.
    ///
    /// This function does not free the `bal_cpu_t`, `bal_allocator_t`, or `bal_memory_interface_t`
    /// passed to the engine.
    ///
    /// # Safety
    ///
    /// * Must NOT be called while the engine is actively compiling code via
    ///  [`bal_engine_run_thread`].
    /// * `engine` must be a valid pointer.
    BAL_COLD void bal_engine_destroy(bal_engine_t *engine);

    /// Checks if the engine is currently executing guest code.
    ///
    /// This function is thread-safe and can be called from any thread.
    ///
    /// # Safety
    ///
    /// `engine` must be a valid pointer.
    BAL_HOT bool bal_engine_is_running(bal_engine_t *engine);

    /// Requests the engine to clear its compiled code cache and JIT buffers.
    ///
    /// This function is completely lock-free and thread-safe.
    ///
    ///  # Safety
    ///
    /// `engine` must be a valid pointer.
    BAL_HOT void bal_engine_clear_cache(bal_engine_t *engine);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BALLISTIC_ENGINE_H */

/*** end of file ***/
