#include "warp.h"

void
sm86_warp_execute(sm86_warp_t *POUND_RESTRICT                      warp,
                  const sm86_decoded_instruction_t *POUND_RESTRICT instructions,
                  const uint32_t                                   max_cycles)
{
    uint32_t                                         pc                  = warp->pc;
    const uint32_t                                   execution_mask      = warp->execution_mask;
    uint32_t                                         cycles              = 0;
    const sm86_decoded_instruction_t *POUND_RESTRICT instructions_cursor = &instructions[pc];

    while (POUND_LIKELY(cycles < max_cycles && execution_mask != 0))
    {
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
                    uint32_t *POUND_RESTRICT destination_cursor
                        = warp->gprs[inst.destination_register];
                    const uint32_t *POUND_RESTRICT source0_cursor
                        = warp->gprs[inst.source0_register];
                    const uint32_t *POUND_RESTRICT source1_cursor
                        = warp->gprs[inst.source1_register];
                    const uint32_t *POUND_RESTRICT source2_cursor
                        = warp->gprs[inst.source2_register];
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

/*** end of file ***/