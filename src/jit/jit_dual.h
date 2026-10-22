#ifndef POUND_JIT_DUAL_H
#define POUND_JIT_DUAL_H

#include "attributes.h"
#include "backend/bal_cpu.h"
#include "bal_engine.h"
#include "bal_memory.h"
#include "guest_memory.h"

POUND_ALIGNED(64) typedef struct
{
    uint64_t ballistic_instructions;
    uint64_t unicorn_instructions;
    uint64_t fallback_count;
    uint64_t resume_count;
    char     pad[32];
} jit_dual_stats_t;

POUND_ALIGNED(64) typedef struct
{
    bal_cpu_t              cpu;
    bal_engine_t           ballistic_engine;
    bal_memory_interface_t ballistic_memory_interface;
    guest_memory_t        *guest_memory;
    bal_allocator_t        allocator;
    uint32_t               engine_flags;
    volatile bool          stop_requested;
    bool                   initialized;
    char                   pad[2];
    jit_dual_stats_t       stats;
} jit_dual_state_t;

#endif // POUND_JIT_DUAL_H
