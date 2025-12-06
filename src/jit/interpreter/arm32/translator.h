#ifndef POUND_JIT_INTERPRETER_TRANSLATOR_H
#define POUND_JIT_INTERPRETER_TRANSLATOR_H

#include "instruction.h"
#include "frontend/decoder/arm32.h"
#include "host/memory/arena.h"

/*!
 * @brief Result codes for the ARM32 interpreter translation process.
 */
typedef enum
{
    PVM_JIT_INTERPRETER_ARM32_TRANSLATE_SUCCESS = 0,
    PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OUT_OF_MEMORY,
    PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OVERFLOW,
    PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_MEMORY_ALLOCATION_FAILED,
} pvm_jit_interpreter_arm32_translate_result_t;

/*!
 * @brief Translates a sequence of raw ARM32 machine code into an executable interpreter block.
 *
 * @details
 * This function performs a linear sweep of the provided `guest_code`, decoding
 * each instruction and generating a corresponding internal interpreter format.
 *
 * **Translation Termination:**
 * The translation process continues until one of the following conditions is met:
 * 1. An explicit control flow instruction is encountered (e.g., `B`, `BL`, `BX`).
 * 2. An ALU operation modifies the Program Counter (R15) (e.g., `ADD PC, ...`).
 * 3. The `max_instructions` limit is reached.
 *
 * @param[in,out] arena
 *      The memory arena used to allocate the resulting instruction array.
 *      Must be initialized and have sufficient remaining capacity.
 *
 * @param[out] block
 *      Pointer to the block structure to populate. On `SUCCESS`:
 *      - `block->instructions` points to valid memory inside `arena`.
 *      - `block->count` contains the number of instructions plus the sentinel.
 *      On failure, `block` contents are undefined but safe (pointers set to NULL).
 *
 * @param[in] guest_code
 *      Pointer to the buffer containing raw Little Endian 32-bit ARM opcodes.
 *      Must not be NULL.
 *
 * @param[in] max_instructions
 *      The hard limit on the number of instructions to translate.
 *
 * @return pvm_jit_interpreter_arm32_translate_result_t
 *      - `SUCCESS`: Translation completed and memory allocated.
 *      - `ERROR_OUT_OF_MEMORY`: The `arena` capacity was insufficient.
 *      - `ERROR_OVERFLOW`: Internal size calculations resulted in integer overflow.
 *      - `ERROR_MEMORY_ALLOCATION_FAILED`: 
 *          Allocated memory for instruction array returned NULL.
 *
 * @warning The lifetime of the returned `block->instructions` is tied to the
 *          lifetime of the `arena`. Resetting or freeing the arena invalidates
 *          the block.
 */
pvm_jit_interpreter_arm32_translate_result_t
pvm_jit_interpreter_arm32_translate (pvm_host_memory_arena_t *restrict arena,
                                     pvm_jit_interpreter_arm32_block_t *block,
                                     const uint32_t *restrict guest_code,
                                     const size_t max_instructions);
#endif // POUND_JIT_INTERPRETER_TRANSLATOR_H
