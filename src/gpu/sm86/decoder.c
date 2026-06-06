#include "decoder.h"
#include "attributes.h"
#include <string.h>

void
sm86_decode(const sm86_raw_instruction_t *POUND_RESTRICT raw_instruction,
            sm86_decoded_instruction_t *POUND_RESTRICT   out_instruction)
{
    sm86_decoded_instruction_t instruction = { 0 };
    const uint64_t             raw_low     = raw_instruction->low;
    const uint64_t             raw_high    = raw_instruction->high;
    const uint32_t             word0       = raw_low & 0xFFFFFFFFUL;
    const uint32_t             word1       = raw_low >> 32;
    const uint32_t             word2       = raw_high & 0xFFFFFFFFUL;
    const uint32_t             word3       = raw_high >> 32;
    const uint16_t             raw_opcode  = word0 & 0x0FFF;

    instruction.opcode               = g_sm86_opcodes_bits_to_enum[raw_opcode];
    instruction.form                 = word0 >> 9 & 0x07;
    instruction.predicate_register   = word0 >> 12 & 0x07;
    instruction.predicate_not        = word0 >> 15 & 0x01;
    instruction.destination_register = word0 >> 16 & 0xFF;
    instruction.source0_register     = word0 >> 24 & 0xFF;
    instruction.delay_cycles         = word3 >> 9 & 0x0F;
    instruction.yield_flag           = word3 >> 13 & 0x01;
    instruction.read_barrier         = word3 >> 14 & 0x07;
    instruction.write_barrier        = word3 >> 17 & 0x07;

    if (4 == instruction.form)
    {
        // Safely cast word1 to int32_t with type punning.
        const union
        {
            uint32_t u;
            int32_t  i;
        } pun                               = { .u = word1 };
        instruction.payload.immediate_value = pun.i;
    }
    else if (5 == instruction.form)
    {
        instruction.payload.constant_buffer.byte_offset   = word1 & 0xFFFF;
        instruction.payload.constant_buffer.binding_index = word1 >> 16 & 0xFF;
    }
    else
    {
        instruction.source1_register = word1 & 0xFF;
    }

    instruction.source2_register = word2 & 0xFF;
    const sm86_instruction_metadata_t *POUND_RESTRICT metadata
        = &g_sm86_opcode_metadata[instruction.opcode];

    switch (metadata->instruction_class)
    {
        case SM86_CLASS_FLOAT_ALU:
        case SM86_CLASS_HALF_FLOAT_ALU:
            instruction.source0_neg = word2 >> 8 & 0x01;
            instruction.source0_abs = word2 >> 9 & 0x01;
            instruction.source1_abs = word1 >> 30 & 0x01;
            instruction.source1_neg = word1 >> 31 & 0x01;
            instruction.saturate    = word2 >> 13 & 0x01;
            instruction.ftz         = word2 >> 16 & 0x01;
            instruction.is_uniform  = word2 >> 27 & 0x01;
            break;
        case SM86_CLASS_INT_ALU:
            instruction.source0_neg   = word2 >> 8 & 0x01;
            instruction.source1_neg   = word1 >> 31 & 0x01;
            instruction.is_uniform    = word2 >> 27 & 0x01;
            instruction.extended_math = word2 >> 10 & 0x01;

            if (SM86_OPCODE_ISETP == instruction.opcode)
            {
                instruction.cmp_type                  = word2 >> 9 & 0x01;
                instruction.bool_operator             = word2 >> 10 & 0x03;
                instruction.cmp_operator              = word2 >> 12 & 0x07;
                instruction.destination_register      = word2 >> 17 & 0x07;
                instruction.accumulator_predicate     = word2 >> 23 & 0x07;
                instruction.accumulator_predicate_not = word2 >> 26 & 0x01;
            }

            break;
        case SM86_CLASS_MEMORY_LOAD_STORE:
        case SM86_CLASS_TEXTURE_FETCH:
        case SM86_CLASS_SURFACE_ATOMIC:
        case SM86_CLASS_CONTROL_FLOW:
            instruction.is_uniform            = word2 >> 27 & 0x01;
            instruction.payload.memory_offset = (int32_t)word1 >> 8;
            break;
        case SM86_CLASS_SYNC_AND_YIELD:
            // Handled by the universal barrier bits in word 3.
            //
            break;
        case SM86_CLASS_UNKNOWN:
        default:
            break;
    }

    *out_instruction = instruction;
}
void
sm86_decode_block(const sm86_raw_instruction_t *POUND_RESTRICT raw_instructions,
                  sm86_decoded_instruction_t *POUND_RESTRICT   out_decoded_instructions,
                  const uint32_t                               instruction_count)
{
    const sm86_raw_instruction_t *POUND_RESTRICT raw_cursor     = raw_instructions;
    sm86_decoded_instruction_t *POUND_RESTRICT   decoded_cursor = out_decoded_instructions;

    for (uint32_t i = 0; i < instruction_count; ++i)
    {
        sm86_decode(raw_cursor++, decoded_cursor++);
    }
}

/*** end of file ***/
