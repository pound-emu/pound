#ifndef BALLISTIC_BAL_ENGINE_FLAGS_H
#define BALLISTIC_BAL_ENGINE_FLAGS_H

// If this is disabled and Ballistic segfaults running compiled code, you will not be able to track
// where in the JIT buffer caused Ballistic to crash. See bal_jit_debug.h.
#define BAL_ENGINE_FLAG_JIT_DEBUG (1 << 0)

/// Forces the engine to return to the host after executing a single instruction.
#define BAL_ENGINE_FLAG_SINGLE_STEP (1U << 1U)

/// Tells the engine an external interrupt is pending. Engine will exit at the next safe boundary.
#define BAL_ENGINE_FLAG_INTERRUPT_PENDING (1U << 2U)

/// Wait for Interrupt yields the host thread instead of acting as a NOP.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_WFI_YIELDS_HOST (1U << 3U)

/// Disables Tier 2 (Optimizing) compilation. Only use Tier 1.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_DISABLE_TIER2 (1U << 4U)

/// Bypass block cache and force recompilation of every block.
#define BAL_ENGINE_FLAG_DISABLE_BLOCK_CACHE (1U << 5U)

/// Enables fast-math optimizations in Ballistic. Deviates slightly from strict ARM compliance.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_FAST_MATH (1U << 6U)

/// Tracks the number of ARM executed instructions via `instruction_count` in [`bal_cpu_t`]. Adds a
/// performance penalty.
#define BAL_ENGINE_FLAG_INSTRUCTION_COUNTING (1U << 7U)

/// Force strict memory alignment checks for all guest memory accesses. Traps on unaligned.
#define BAL_ENGINE_FLAG_STRICT_ALIGNMENT (1U << 8U)

/// Traps on Supervisor Call (SVC) instruction and returns control to the host.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_TRAP_SVC (1U << 9U)

/// Traps on Secure Monitor Call (SMC) instruction or Hypervisor Call (HVC) instructions.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_TRAP_SMC_HVC (1U << 10U)

/// Emits a callout to the host before executing any compiled basic block.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_ENABLE_BLOCK_HOOKS (1U << 11U)

/// Emits a callout to the host on every memory read/write.
/// NOT IMPLEMENTED YET.
#define BAL_ENGINE_FLAG_ENABLE_MEMORY_HOOKS (1U << 12U)

/// Enables verbose logging of every basic block executed.
#define BAL_ENGINE_FLAG_LOG_BLOCKS (1U << 13U)

#endif // BALLISTIC_BAL_ENGINE_FLAGS_H
