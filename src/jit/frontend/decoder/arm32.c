#include "arm32.h"
#include "arm32_table_generated.h"
#include "common/passert.h"
#include <string.h>
#include <stdbool.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

const pvm_jit_decoder_arm32_instruction_info_t *
pvm_jit_decoder_arm32_decode (const uint32_t instruction)
{
    /* Extract hash key: Bits [27:20] and [7:4] */
    const uint32_t major = (instruction >> 20U) & 0xFFU;
    const uint32_t minor = (instruction >> 4U) & 0xFU;

    const uint16_t index = (uint16_t)((major << 4U) | minor);

    const decode_bucket_t *bucket = &g_decoder_lookup_table[index];

    for (size_t i = 0; i < bucket->count; ++i)
    {
        const pvm_jit_decoder_arm32_instruction_info_t *info = bucket->instructions[i];

        if ((instruction & info->mask) == info->expected)
        {
            return info;
        }
    }

    return NULL;
}

