#include "instruction.h"
#include "opcode.h"
#include "common/passert.h"
#include "host/memory/arena.h"
#include <stddef.h>
#include <memory.h>

#define LOG_MODULE "jit"
#include "logging.h"

namespace pound::jit::ir {
typedef struct
{
    instruction_t *instructions;
    uint64_t       start_location;
    uint64_t       end_location;
    size_t         instruction_size;
    size_t         instruction_capacity;
} basic_block_t;

void
basic_block_init (pound::host::memory::arena_t *allocator,
                  basic_block_t                *block,
                  uint64_t                      start_location,
                  size_t                        instruction_capacity)
{
    PVM_ASSERT(nullptr != allocator);
    PVM_ASSERT(nullptr != allocator->data);
    PVM_ASSERT(nullptr != block);
    PVM_ASSERT(allocator->size < allocator->capacity);

    block->instructions         = nullptr;
    block->start_location       = start_location;
    block->end_location         = start_location;
    block->instruction_size     = 0;
    block->instruction_capacity = instruction_capacity;
    block->instructions = (instruction_t *)pound::host::memory::arena_allocate(
        allocator, sizeof(instruction_t) * instruction_capacity);

    PVM_ASSERT(nullptr != block->instructions);
    LOG_TRACE("Allocated %d bytes to basic block instructions array", sizeof(instruction_t) * instruction_capacity);
}

void
basic_block_append (basic_block_t *basic_block,
                    const opcode_t opcode,
                    const value_t  args[MAX_IR_ARGS])
{
    PVM_ASSERT(nullptr != basic_block);
    PVM_ASSERT(basic_block->instruction_size < basic_block->instruction_capacity);
    instruction_t *instruction
        = &basic_block->instructions[basic_block->instruction_size];
    PVM_ASSERT(nullptr != instruction);


    LOG_TRACE("Appending opcode %s to basic block", instruction->opcode);
    instruction->opcode = opcode;
    if (nullptr != args)
    {
        (void)memcpy(instruction->args, args, sizeof(value_t) * MAX_IR_ARGS);
    }
    else
    {
        (void)memset(instruction->args, 0, sizeof(value_t) * MAX_IR_ARGS);
    }
    ++basic_block->instruction_size;
}
}
