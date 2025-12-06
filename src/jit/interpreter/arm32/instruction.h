#ifndef POUND_JIT_INTERPRETER_INSTRUCTION_H
#define POUND_JIT_INTERPRETER_INSTRUCTION_H
#include "frontend/decoder/arm32_opcodes.h"
#include <stdint.h>
#include <stddef.h>

/*!
 * @brief Interpreter instruction format.
 * @details
 * We store the raw instruction alongside the opcode. This allows handlers
 * to perform fast bit-shifting to extract operands on demand, avoiding the
 * memory overhead of storing every possible operand field in this struct
 * which would ruin CPU cache locality.
 */
typedef struct
{
    pvm_jit_decoder_arm32_opcode_t opcode;
    uint32_t raw;
} pvm_jit_interpreter_arm32_instruction_t;

typedef struct
{
    pvm_jit_interpreter_arm32_instruction_t* instructions;
    size_t count;
} pvm_jit_interpreter_arm32_block_t;

void temp(void);

#endif // POUND_JIT_INTERPRETER_INSTRUCTION_H
       // h9hj
