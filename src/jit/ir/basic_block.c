#include "instruction.h"
#include "opcode.h"
#include "common/passert.h"
#include "host/memory/arena.h"
#include <stddef.h>
#include <memory.h>

#define LOG_MODULE "jit"
#include "logging.h"

typedef struct
{
    pvm_jit_ir_instruction_t *instructions;
    uint64_t                  start_location;
    uint64_t                  end_location;
    size_t                    instruction_size;
    size_t                    instruction_capacity;
} pvm_jit_ir_basic_block_t;

void
pvm_jit_ir_basic_block_init (pvm_host_memory_arena_t  *allocator,
                             pvm_jit_ir_basic_block_t *block,
                             uint64_t                  start_location,
                             size_t                    instruction_capacity)
{
    PVM_ASSERT(NULL != allocator);
    PVM_ASSERT(NULL != allocator->data);
    PVM_ASSERT(NULL != block);
    PVM_ASSERT(allocator->size < allocator->capacity);

    block->instructions         = NULL;
    block->start_location       = start_location;
    block->end_location         = start_location;
    block->instruction_size     = 0;
    block->instruction_capacity = instruction_capacity;
    block->instructions
        = (pvm_jit_ir_instruction_t *)pvm_host_memory_arena_allocate(
            allocator, sizeof(pvm_jit_ir_instruction_t) * instruction_capacity);

    PVM_ASSERT(NULL != block->instructions);
    LOG_TRACE("Allocated %d bytes to basic block instructions array",
              sizeof(pvm_jit_ir_instruction_t) * instruction_capacity);
}

void
pvm_jit_ir_basic_block_append (pvm_jit_ir_basic_block_t *basic_block,
                               const pvm_jit_ir_opcode_t opcode,
                               const pvm_jit_ir_value_t  args[MAX_IR_ARGS])
{
    PVM_ASSERT(NULL != basic_block);
    PVM_ASSERT(basic_block->instruction_size
               < basic_block->instruction_capacity);
    pvm_jit_ir_instruction_t *instruction
        = &basic_block->instructions[basic_block->instruction_size];
    PVM_ASSERT(NULL != instruction);

    LOG_TRACE("Appending opcode %s to basic block", instruction->opcode);
    instruction->opcode = opcode;
    if (NULL != args)
    {
        (void)memcpy(
            instruction->args, args, sizeof(pvm_jit_ir_value_t) * MAX_IR_ARGS);
    }
    else
    {
        (void)memset(
            instruction->args, 0, sizeof(pvm_jit_ir_value_t) * MAX_IR_ARGS);
    }
    ++basic_block->instruction_size;
}
