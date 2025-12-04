#include "instruction.h"
#include "common/passert.h"
#include <stddef.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

pvm_jit_ir_value_t *
pvm_jit_ir_instruction_get_arg (pvm_jit_ir_instruction_t *instruction,
                                size_t                    arg_index)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    pvm_jit_ir_value_t *arg = &instruction->args[arg_index];
    PVM_ASSERT(NULL != arg);
    return arg;
}

uint64_t
pvm_jit_ir_instruction_get_arg_u64 (pvm_jit_ir_instruction_t *instruction,
                                    const size_t              arg_index)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    pvm_jit_ir_value_t *arg
        = pvm_jit_ir_instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(NULL != arg);

    PVM_ASSERT(IR_TYPE_U64 == arg->type);
    uint64_t value = pvm_jit_ir_value_get_u64(arg);
    return value;
}

uint32_t
pvm_jit_ir_instruction_get_arg_u32 (pvm_jit_ir_instruction_t *instruction,
                                    const size_t              arg_index)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    pvm_jit_ir_value_t *arg
        = pvm_jit_ir_instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(NULL != arg);

    PVM_ASSERT(IR_TYPE_U32 == arg->type);
    uint32_t value = pvm_jit_ir_value_get_u32(arg);
    return value;
}

uint8_t
pvm_jit_ir_instruction_get_arg_u8 (pvm_jit_ir_instruction_t *instruction,
                                   const size_t              arg_index)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    pvm_jit_ir_value_t *arg
        = pvm_jit_ir_instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(NULL != arg);

    PVM_ASSERT(IR_TYPE_U8 == arg->type);
    uint8_t value = pvm_jit_ir_value_get_u8(arg);
    return value;
}

bool
pvm_jit_ir_instruction_get_arg_u1 (pvm_jit_ir_instruction_t *instruction,
                                   const size_t              arg_index)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    pvm_jit_ir_value_t *arg
        = pvm_jit_ir_instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(NULL != arg);

    PVM_ASSERT(IR_TYPE_U1 == arg->type);
    uint8_t value = pvm_jit_ir_value_get_u1(arg);
    return value;
}

pvm_jit_a32_register_t
pvm_jit_ir_instruction_get_arg_a32_register (
    pvm_jit_ir_instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    pvm_jit_ir_value_t *arg
        = pvm_jit_ir_instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(NULL != arg);

    PVM_ASSERT(IR_TYPE_A32_REGISTER == arg->type);
    pvm_jit_a32_register_t value = pvm_jit_ir_value_get_a32_register(arg);
    return value;
}

pvm_jit_ir_type_t
pvm_jit_ir_instruction_get_return_type (pvm_jit_ir_instruction_t *instruction)
{
    PVM_ASSERT(NULL != instruction);
    PVM_ASSERT(instruction->opcode < NUM_OPCODE);

    pvm_jit_ir_decoded_opcode_t *decoded_opcode
        = &g_pvm_jit_ir_opcodes[instruction->opcode];
    PVM_ASSERT(NULL != decoded_opcode);

    return decoded_opcode->type;
}

const char *
pvm_jit_ir_instruction_get_opcode_name (
    const pvm_jit_ir_instruction_t *instruction)
{
    PVM_ASSERT(NULL != instruction);

    pvm_jit_ir_decoded_opcode_t *decoded_opcode
        = &g_pvm_jit_ir_opcodes[instruction->opcode];
    PVM_ASSERT(NULL != decoded_opcode);

    const char *name = decoded_opcode->name;
    PVM_ASSERT(NULL != name);

    return name;
}
