#ifndef POUND_JIT_DECODER_ARM32_H
#define POUND_JIT_DECODER_ARM32_H

#include <stddef.h>
#include <stdint.h>
#include "host/memory/arena.h"

namespace pound::jit::decoder {
typedef struct arm32_decoder arm32_decoder_t;

typedef struct
{
    const char *p_name;
    uint32_t    mask;
    uint32_t    expected;
} arm32_instruction_info_t;

struct arm32_decoder
{
    pound::host::memory::arena_t allocator;
    arm32_instruction_info_t    *p_instructions;
    size_t                       instruction_count;
    size_t                       instruction_capacity;
};

void                      arm32_init(pound::host::memory::arena_t allocator,
                                     arm32_decoder_t             *p_decoder);
arm32_instruction_info_t *arm32_decode(arm32_decoder_t *p_decoder,
                                       uint32_t         instruction);
}
#endif // POUND_JIT_DECODER_ARM32_H
