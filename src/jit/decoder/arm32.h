#ifndef POUND_JIT_DECODER_ARM32_H
#define POUND_JIT_DECODER_ARM32_H

#include <stddef.h>
#include <stdint.h>
#include "host/memory/arena.h"

namespace pound::jit::decoder
{
typedef uint32_t arm32_opcode_t;
typedef uint32_t arm32_instruction_t;

typedef struct arm32_decoder arm32_decoder_t;
typedef void (*arm32_handler_fn)(arm32_decoder_t* decoder, arm32_instruction_t instruction);

typedef struct
{
    const char* name;
    arm32_opcode_t mask;
    arm32_opcode_t expected;
    arm32_handler_fn handler;

    /* Use to order instructions in the lookup table. The more specific
     * instructions are checked first */
    uint8_t priority;
} arm32_instruction_info_t;

struct arm32_decoder
{
    pound::host::memory::arena_t allocator;
    arm32_instruction_info_t* instructions;
    size_t instruction_count;
    size_t instruction_capacity;

    struct
    {
        arm32_instruction_info_t** bucket;
        size_t count;
        size_t capacity;
    } lookup_table[4096]; /* 2^12 entries. */
};

extern arm32_decoder_t g_arm32_decoder;

void arm32_init(pound::host::memory::arena_t allocator, arm32_decoder_t* decoder);

void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, arm32_opcode_t mask, arm32_opcode_t expected,
                           arm32_handler_fn handler);
void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction);

void arm32_log_instruction_info(const arm32_instruction_info_t* info);

}  // namespace pound::jit::decoder
#endif  // POUND_JIT_DECODER_ARM32_H