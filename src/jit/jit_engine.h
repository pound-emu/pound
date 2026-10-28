#ifndef POUND_JIT_DUAL_H
#define POUND_JIT_DUAL_H

#include "attributes.h"
#include "backend/bal_cpu.h"
#include "bal_engine.h"
#include "bal_memory.h"
#include "guest_memory.h"
#include "unicorn/unicorn.h"

POUND_ALIGNED(64) typedef struct
{
    uint64_t ballistic_instructions;
    uint64_t unicorn_instructions;
    uint64_t fallback_count;
    uint64_t resume_count;
    char     pad[32];
} jit_stats_t;

POUND_ALIGNED(64) typedef struct
{
    bal_cpu_t              cpu;
    bal_engine_t           ballistic_engine;
    bal_memory_interface_t ballistic_memory_interface;
    guest_memory_t        *guest_memory;
    bal_allocator_t        allocator;

    // NULL until the first fallback triggers.
    uc_engine *unicorn_engine;
    uc_hook    unicorn_memory_hook;
    bool       unicorn_initialized;
    char       pad0[3];

    uint32_t      engine_flags;
    volatile bool stop_requested;
    bool          initialized;
    char          pad1[2];
    error_t       status;
    char          pad2[16];
    jit_stats_t   stats;
} jit_engine_t;

POUND_COLD error_t jit_engine_init(jit_engine_t *POUND_RESTRICT   context,
                                   guest_memory_t *POUND_RESTRICT guest_memory,
                                   uint32_t                       flags);

// Thread Safe.
POUND_HOT void jit_engine_stop(jit_engine_t *POUND_RESTRICT engine);

POUND_COLD error_t jit_engine_reset(jit_engine_t *POUND_RESTRICT engine);

#endif // POUND_JIT_DUAL_H
