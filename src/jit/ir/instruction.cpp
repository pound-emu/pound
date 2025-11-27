#include "instruction.h"
#include "common/passert.h"
#include <stddef.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::ir {
value_t *
instruction_get_arg (instruction_t *instruction, size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    value_t *arg = &instruction->args[arg_index];
    PVM_ASSERT(nullptr != arg);
    return arg;
}

uint64_t
instruction_get_arg_u64 (instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    value_t *arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U64 == arg->type);
    uint64_t value = value_get_u64(arg);
    return value;
}

uint32_t
instruction_get_arg_u32 (instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    value_t *arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U32 == arg->type);
    uint32_t value = value_get_u32(arg);
    return value;
}

uint8_t
instruction_get_arg_u8 (instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    value_t *arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U8 == arg->type);
    uint8_t value = value_get_u8(arg);
    return value;
}

bool
instruction_get_arg_u1 (instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    value_t *arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U1 == arg->type);
    uint8_t value = value_get_u1(arg);
    return value;
}

pound::jit::a32_register_t
instruction_get_arg_a32_register (instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    value_t *arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_A32_REGISTER == arg->type);
    pound::jit::a32_register_t value = value_get_a32_register(arg);
    return value;
}

type_t
instruction_get_return_type (instruction_t *instruction)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(instruction->opcode < NUM_OPCODE);

    decoded_opcode_t *decoded_opcode = &g_opcodes[instruction->opcode];
    PVM_ASSERT(nullptr != decoded_opcode);

    return decoded_opcode->type;
}

const char *
instruction_get_opcode_name (const instruction_t *instruction)
{
    PVM_ASSERT(nullptr != instruction);

    decoded_opcode_t *decoded_opcode = &g_opcodes[instruction->opcode];
    PVM_ASSERT(nullptr != decoded_opcode);

    const char *name = decoded_opcode->name;
    PVM_ASSERT(nullptr != name);

    return name;
}
}
