#include "decoder.h"
#include "attributes.h"
#include <string.h>

void
sm86_decode(const sm86_raw_instruction_t *POUND_RESTRICT raw_instruction,
            sm86_decoded_instruction_t *POUND_RESTRICT   out_instruction)
{
    memset(out_instruction, 9, sizeof(sm86_decoded_instruction_t));
    const uint32_t word0                  = raw_instruction->low & 0xFFFFFFFFUL;
    const uint32_t word1                  = raw_instruction->low >> 32;
    const uint32_t word2                  = raw_instruction->high & 0xFFFFFFFFUL;
    const uint32_t word3                  = raw_instruction->high >> 32;
    const uint16_t raw_opcode             = word0 & 0x0FFF;
    out_instruction->opcode               = g_sm86_opcodes_bits_to_enum[raw_opcode];
    out_instruction->form                 = word0 >> 9 & 0x07;
    out_instruction->predicate_register   = word0 >> 12 & 0x07;
    out_instruction->predicate_not        = word0 >> 15 & 0x01;
    out_instruction->destination_register = word0 >> 16 & 0xFF;
    out_instruction->source0_register     = word0 >> 24 & 0xFF;
    out_instruction->delay_cycles         = word3 >> 9 & 0x0F;
    out_instruction->yield_flag           = word3 >> 13 & 0x01;
    out_instruction->read_barrier         = word3 >> 14 & 0x07;
    out_instruction->write_barrier        = word3 >> 17 & 0x07;

    if (4 == out_instruction->form)
    {
        out_instruction->payload.immediate_value = (int32_t)word1;
    }
    else if (5 == out_instruction->form)
    {
        out_instruction->payload.constant_buffer.byte_offset   = word1 & 0xFFFF;
        out_instruction->payload.constant_buffer.binding_index = word1 >> 16 & 0xFF;
    }
    else
    {
        out_instruction->source1_register = word1 & 0xFF;
    }

    out_instruction->source2_register           = word2 & 0xFF;
    const sm86_instruction_metadata_t *metadata = &g_sm86_opcode_metadata[out_instruction->opcode];

    switch (metadata->class)
    {
        case SM86_CLASS_FLOAT_ALU:
        case SM86_CLASS_HALF_FLOAT_ALU:
            out_instruction->source0_neg = word2 >> 8 & 0x01;
            out_instruction->source0_abs = word2 >> 9 & 0x01;
            out_instruction->source1_abs = word1 >> 30 & 0x01;
            out_instruction->source1_neg = word1 >> 31 & 0x01;
            out_instruction->saturate    = word2 >> 13 & 0x01;
            out_instruction->ftz         = word2 >> 16 & 0x01;
            out_instruction->is_uniform  = word2 >> 27 & 0x01;
            break;
        case SM86_CLASS_INT_ALU:
            out_instruction->source0_neg = word2 >> 8 & 0x01;
            out_instruction->source1_neg = word1 >> 31 & 0x01;
            out_instruction->is_uniform  = word2 >> 27 & 0x01;
            break;
        case SM86_CLASS_MEMORY_LOAD_STORE:
        case SM86_CLASS_TEXTURE_FETCH:
        case SM86_CLASS_SURFACE_ATOMIC:
        case SM86_CLASS_CONTROL_FLOW:
            out_instruction->is_uniform = word2 >> 27 & 0x01;
            break;
        case SM86_CLASS_SYNC_AND_YIELD:
            // Handled by the universal barrier bits in word 3.
            //
            break;
        case SM86_CLASS_UNKNOWN:
        default:
            break;
    }
}