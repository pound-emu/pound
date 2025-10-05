#ifndef POUND_JIT_DECODER_ARM32_H
#define POUND_JIT_DECODER_ARM32_H

#include <stddef.h>
#include <stdint.h>

namespace pound::jit::decoder
{
typedef uint32_t arm32_opcode_t;
typedef uint32_t arm32_instruction_t;

typedef struct arm32_instruction_info arm32_instruction_info_t;
typedef struct arm32_decoder arm32_decoder_t;

extern arm32_decoder_t g_arm32_decoder;

typedef void (*arm32_handler_fn)(arm32_decoder_t* decoder, arm32_instruction_t instruction);

struct a32_instruction_info
{
    const char* name;
    arm32_opcode_t mask;
    arm32_opcode_t expected;
    arm32_handler_fn handler;
    uint8_t priority; /* Higher = more specific */
};

struct arm32_decoder
{
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

void arm32_init(arm32_decoder_t* decoder);

void arm32_add_instruction(arm32_decoder_t* decoder, const char* nane, arm32_opcode_t mask, arm32_opcode_t expected,
                           arm32_handler_fn handler);
void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction);

}  // namespace pound::jit::decoder
#endif  // POUND_JIT_DECODER_ARM32_H
