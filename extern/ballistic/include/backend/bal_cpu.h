//! Holds the state of the ARM guest CPU.

#ifndef BALLISTIC_BAL_CPU_H
#define BALLISTIC_BAL_CPU_H

#include "bal_attributes.h"
#include "bal_jit_debug.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

    BAL_ALIGNED(64) typedef struct
    {
        uint64_t                 x[32];
        uint64_t                 pc;
        bal_jit_debug_context_t *debug_context;

        // ARM's Carry flag is inverted compared to x86's Carry during subtraction operations.
        // Compiler's must handle this.
        uint8_t  flag_c; // ARM C -> x86 CF
        uint8_t  flag_z; // ARM Z -> x86 ZF
        uint8_t  flag_n; // ARM N -> x86 SF
        uint8_t  flag_v; // ARM V -> x86 OF
        uint32_t _pad_flags;

        /// Tracks executed guest instructions.
        ///
        /// This only increments if the engine flag [`BAL_ENGINE_FLAG_INSTRUCTION_COUNTING`] is set.
        uint64_t instruction_count;
        char     pad[32];
    } bal_cpu_t;

    /// The function signature of a JIT-compiled basic block.
    typedef void (*bal_jit_block_t)(bal_cpu_t *cpu);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BALLISTIC_BAL_CPU_H

/*** end of file ***/
