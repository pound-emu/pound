#include "instruction.h"
#include "common/passert.h"
#include <stddef.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::ir {

const value_t*
instruction_get_arg (const instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    const value_t *arg = &instruction->args[arg_index];
    PVM_ASSERT(nullptr != arg);
    return arg;
}

const uint64_t
instruction_get_arg_u64(const instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    const value_t* arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U64 == arg->type);
    const uint64_t value = value_get_u64(arg);
    return value;
}

const uint32_t
instruction_get_arg_u32(const instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    const value_t* arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U32 == arg->type);
    const uint32_t value = value_get_u32(arg);
    return value;
}

const uint8_t
instruction_get_arg_u8(const instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    const value_t* arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U8 == arg->type);
    const uint8_t value = value_get_u8(arg);
    return value;
}

const bool
instruction_get_arg_u1(const instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    const value_t* arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_U1 == arg->type);
    const uint8_t value = value_get_u1(arg);
    return value;
}

const pound::jit::a32_register_t
instruction_get_arg_a32_register(const instruction_t *instruction, const size_t arg_index)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(arg_index < MAX_IR_ARGS);

    const value_t* arg = instruction_get_arg(instruction, arg_index);
    PVM_ASSERT(nullptr != arg);

    PVM_ASSERT(IR_TYPE_A32_REGISTER == arg->type);
    const pound::jit::a32_register_t value = value_get_a32_register(arg);
    return value;
}

const type_t
instruction_get_return_type (const instruction_t *instruction)
{
    PVM_ASSERT(nullptr != instruction);
    PVM_ASSERT(instruction->opcode < NUM_OPCODE);

    const decoded_opcode_t *decoded_opcode = &g_opcodes[instruction->opcode];
    PVM_ASSERT(nullptr != decoded_opcode);

    return decoded_opcode->type;
}

const char*
instruction_get_opcode_name(const instruction_t *instruction) 
{
    PVM_ASSERT(nullptr != instruction);

    const decoded_opcode_t *decoded_opcode = &g_opcodes[instruction->opcode];
    PVM_ASSERT(nullptr != decoded_opcode);

    const char *name = decoded_opcode->name;
    PVM_ASSERT(nullptr != name);

    return name;
}

void
instruction_list_append (instruction_list_t *list, instruction_t *instruction)
{
    PVM_ASSERT(nullptr != list);
    PVM_ASSERT(nullptr != instruction);

    instruction->next     = nullptr;
    instruction->previous = list->tail;
    if (nullptr != list->tail)
    {
        list->tail->next = instruction;
    }
    else
    {
        list->head = instruction;
    }
    list->tail = instruction;
}

void
instruction_list_remove (instruction_list_t *list, instruction_t *instruction)
{
    PVM_ASSERT(nullptr != list);
    PVM_ASSERT(nullptr != instruction);

    if (nullptr != instruction->previous)
    {
        instruction->previous->next = instruction->next;
    }
    else
    {
        list->head = instruction->next;
    }

    if (nullptr != instruction->next)
    {
        instruction->next->previous = instruction->previous;
    }
    else
    {
        list->tail = instruction->previous;
    }

    instruction->next     = nullptr;
    instruction->previous = nullptr;
}
}
