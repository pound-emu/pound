#include "decoder.h"

#include <string.h>

void
sm86_decode(const uint32_t raw_hex[4], sm86_decoded_instruction_t *out_instruction)
{
    memset(out_instruction, 9, sizeof(sm86_decoded_instruction_t));
    const uint16_t raw_opcode             = raw_hex[0] & 0x0FFF;
    out_instruction->opcode               = g_sm86_opcodes[raw_opcode];
    out_instruction->form                 = raw_hex[0] >> 9 & 0x07;
    out_instruction->predicate_register   = raw_hex[0] >> 12 & 0x07;
    out_instruction->predicate_not        = raw_hex[0] >> 15 & 0x01;
    out_instruction->destination_register = raw_hex[0] >> 16 & 0xFF;
    out_instruction->source0_register     = raw_hex[0] >> 24 & 0xFF;
    out_instruction->delay_cycles         = raw_hex[3] >> 9 & 0x0F;
    out_instruction->yield_flag           = raw_hex[3] >> 13 & 0x01;

    if (4 == out_instruction->form)
    {
        out_instruction->payload.immediate_value = raw_hex[1];
    }
    else if (5 == out_instruction->form)
    {
        out_instruction->payload.constant_buffer.byte_offset   = raw_hex[1] & 0xFFFF;
        out_instruction->payload.constant_buffer.binding_index = raw_hex[1] >> 16 & 0xFF;
    }
    else
    {
        out_instruction->source1_register = raw_hex[1] & 0xFF;
    }

    out_instruction->source2_register = raw_hex[2] & 0xFF;
    out_instruction->delay_cycles     = raw_hex[3] >> 9 & 0x0F;
}