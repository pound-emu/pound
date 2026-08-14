#ifndef BALLISTIC_BAL_X86_TIER1_COMPILER_H
#define BALLISTIC_BAL_X86_TIER1_COMPILER_H

#include "bal_attributes.h"
#include "bal_jit_debug.h"
#include "bal_memory.h"
#include "bal_x86_assembler.h"
#include "bal_x86_sliding_window.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    BAL_ALIGNED(64) typedef struct
    {
        bal_x86_assembler_t      assembler;
        bal_sliding_window_t     window;
        int8_t                   arm_to_x86[32];
        int8_t                   x86_to_arm[16];
        uint32_t                 is_dirty;
        uint32_t                 pad0;
        bal_jit_debug_context_t *debug_context;
        bal_error_t              status;
        uint32_t                 pad1;
    } bal_tier1_compiler_t;

    static_assert(256 == sizeof(bal_tier1_compiler_t), "Struct size mismatch");

    /// Initializes the Tier 1 Compiler.
    ///
    /// # Safety
    ///
    /// The `executable_buffer` must be memory allocated with execution permissions.
    ///
    /// # Errors
    ///
    /// Returns [`BAL_SUCCESS`] on success.
    ///
    /// Returns [`BAL_ERROR_INVALID_ARGUMENT`] if passed pointers are NULL or `buffer_size` is 0.
    BAL_COLD bal_error_t bal_tier1_compiler_init(bal_tier1_compiler_t    *compiler,
                                                 bal_executable_buffer_t  executable_buffer,
                                                 size_t                   buffer_size,
                                                 bal_jit_debug_context_t *debug_context);

    BAL_HOT void bal_tier1_compiler_reset(bal_tier1_compiler_t *compiler);

    BAL_HOT void *bal_tier1_compiler_translate(bal_tier1_compiler_t         *compiler,
                                               const bal_memory_interface_t *memory_interface,
                                               bal_guest_address_t           guest_address,
                                               size_t                        max_instructions,
                                               uint32_t                      engine_flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BALLISTIC_BAL_X86_TIER1_COMPILER_H

/*** end of file ***/
