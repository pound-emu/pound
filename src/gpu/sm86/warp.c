#include "warp.h"

static const uint32_t *sm86_fetch_source1(const sm86_warp_t *POUND_RESTRICT warp,
                                          const sm86_decoded_instruction_t *POUND_RESTRICT
                                                                   instruction,
                                          uint32_t *POUND_RESTRICT temp_buffer);

static const uint32_t *sm86_fetch_source2(const sm86_warp_t *POUND_RESTRICT warp,
                                          const sm86_decoded_instruction_t *POUND_RESTRICT
                                                                   instruction,
                                          uint32_t *POUND_RESTRICT temp_buffer);

void sm86_execute_iadd3(sm86_warp_t                      *warp,
                        const sm86_decoded_instruction_t *inst,
                        uint32_t                          active_threads);

void
sm86_warp_execute(sm86_warp_t *POUND_RESTRICT                      warp,
                  const sm86_decoded_instruction_t *POUND_RESTRICT instructions,
                  const uint32_t                                   max_cycles)
{
    uint32_t pc                  = warp->pc;
    uint32_t execution_mask      = warp->execution_mask;
    uint32_t reconvergence_depth = warp->reconvergence_depth;
    uint32_t cycles              = 0;
    const sm86_decoded_instruction_t *POUND_RESTRICT instructions_cursor = &instructions[pc];

    while (POUND_LIKELY(cycles < max_cycles && execution_mask != 0))
    {
        // Pop paths if current paths yielded/exited.
        while (POUND_UNLIKELY(0 == execution_mask && reconvergence_depth > 0))
        {
            --reconvergence_depth;
            pc                  = warp->reconvergence_stack[reconvergence_depth].pc;
            execution_mask      = warp->reconvergence_stack[reconvergence_depth].active_mask;
            instructions_cursor = &instructions[pc];
        }

        if (POUND_UNLIKELY(0 == execution_mask))
        {
            break;
        }

        const sm86_decoded_instruction_t inst           = *instructions_cursor;
        uint32_t                         predicate_mask = warp->predicates[inst.predicate_register];

        if (inst.predicate_not)
        {
            predicate_mask = ~predicate_mask;
        }

        const uint32_t active_threads = execution_mask & predicate_mask;

        if (POUND_LIKELY(active_threads != 0))
        {
            switch (inst.opcode)
            {
                case SM86_OPCODE_IADD3:;
                    sm86_execute_iadd3(warp, &inst, active_threads);

                    break;
                default:
                    break;
            }
        }

        ++pc;
        ++instructions_cursor;
        ++cycles;
    }

    warp->pc             = pc;
    warp->execution_mask = execution_mask;
}

/// Resolves source 1 via splatting.
///
/// If it's a scalar (Imm, CBuf, or UReg), it broadcasts the value into `temp_buffer` and returns
/// `temp_buffer`. Otherwise, it returns the direct pointer to the GPR array.
POUND_HOT static const uint32_t *
sm86_fetch_source1(const sm86_warp_t *POUND_RESTRICT                warp,
                   const sm86_decoded_instruction_t *POUND_RESTRICT instruction,
                   uint32_t *POUND_RESTRICT                         temp_buffer)
{
    if (4 == instruction->form) // Imm32
    {
        const uint32_t           value  = (uint32_t)instruction->payload.immediate_value;
        uint32_t *POUND_RESTRICT cursor = temp_buffer;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            *cursor++ = value;
        }

        return temp_buffer;
    }

    if (6 == instruction->form) // UReg
    {
        const uint32_t           value  = warp->uniform_gprs[instruction->source1_register];
        uint32_t *POUND_RESTRICT cursor = temp_buffer;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            *cursor++ = value;
        }

        return temp_buffer;
    }

    if (5 == instruction->form) // CBuf
    {
        // TODO: Read from Constant Buffer Memory.
        const uint32_t           value  = 0;
        uint32_t *POUND_RESTRICT cursor = temp_buffer;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            *cursor++ = value;
        }

        return temp_buffer;
    }

    return warp->gprs[instruction->source1_register];
}

/// Resolves source 2 via splatting.
///
/// If it's a scalar (Imm, CBuf, or UReg), it broadcasts the value into `temp_buffer` and returns
/// `temp_buffer`. Otherwise, it returns the direct pointer to the GPR array.
POUND_HOT static const uint32_t *
sm86_fetch_source2(const sm86_warp_t *POUND_RESTRICT                warp,
                   const sm86_decoded_instruction_t *POUND_RESTRICT instruction,
                   uint32_t *POUND_RESTRICT                         temp_buffer)
{
    if (2 == instruction->form) // Imm32
    {
        const uint32_t           value  = (uint32_t)instruction->payload.immediate_value;
        uint32_t *POUND_RESTRICT cursor = temp_buffer;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            *cursor++ = value;
        }

        return temp_buffer;
    }

    if (7 == instruction->form) // UReg
    {
        const uint32_t           value  = warp->uniform_gprs[instruction->source2_register];
        uint32_t *POUND_RESTRICT cursor = temp_buffer;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            *cursor++ = value;
        }

        return temp_buffer;
    }

    if (3 == instruction->form) // CBuf
    {
        // TODO: Read from Constant Buffer Memory.
        const uint32_t           value  = 0;
        uint32_t *POUND_RESTRICT cursor = temp_buffer;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            *cursor++ = value;
        }

        return temp_buffer;
    }

    return warp->gprs[instruction->source2_register];
}

POUND_HOT void
sm86_execute_iadd3(sm86_warp_t *POUND_RESTRICT                      warp,
                   const sm86_decoded_instruction_t *POUND_RESTRICT inst,
                   const uint32_t                                   active_threads)
{
    if (POUND_UNLIKELY(inst->is_uniform))
    {
        const uint32_t source0 = warp->uniform_gprs[inst->source0_register];
        uint32_t       source1 = 0;

        if (4 == inst->form)
        {
            source1 = (uint32_t)inst->payload.immediate_value;
        }
        else if (1 == inst->form)
        {
            source1 = warp->uniform_gprs[inst->source1_register];
        }
        else
        {
        }

        uint32_t source2 = 0;

        if (2 == inst->form)
        {
            source2 = (uint32_t)inst->payload.immediate_value;
        }
        else
        {
            source2 = warp->uniform_gprs[inst->source2_register];
        }

        warp->uniform_gprs[inst->destination_register] = source0 + source1 + source2;
    }
    else
    {
        // Allocate 128 bytes on the stack for splatting.
        POUND_ALIGNED(64) uint32_t temp_source1[SM86_WARP_SIZE];
        POUND_ALIGNED(64) uint32_t temp_source2[SM86_WARP_SIZE];

        uint32_t *POUND_RESTRICT       destination_cursor = warp->gprs[inst->destination_register];
        const uint32_t *POUND_RESTRICT source0_cursor     = warp->gprs[inst->source0_register];
        const uint32_t *POUND_RESTRICT source1_cursor
            = sm86_fetch_source1(warp, inst, temp_source1);
        const uint32_t *POUND_RESTRICT source2_cursor
            = sm86_fetch_source2(warp, inst, temp_source2);
        uint32_t thread_mask = active_threads;

        for (int i = 0; i < SM86_WARP_SIZE; ++i)
        {
            const uint32_t source0 = *source0_cursor++;
            const uint32_t source1 = *source1_cursor++;
            const uint32_t source2 = *source2_cursor++;

            if (thread_mask & 1)
            {
                *destination_cursor = source0 + source1 + source2;
            }

            ++destination_cursor;
            thread_mask >>= 1;
        }
    }
}

/*** end of file ***/