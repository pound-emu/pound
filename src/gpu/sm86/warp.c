#include "warp.h"
#include <stdbool.h>

/// Generates a comparison loop.
#define SM86_EVAL_CMP(TYPE, OP_SYMBOL)                                   \
    do                                                                   \
    {                                                                    \
        const uint32_t *POUND_RESTRICT source0_current = source0_cursor; \
        const uint32_t *POUND_RESTRICT source1_current = source1_cursor; \
        uint32_t                       t_mask          = thread_mask;    \
        for (int i = 0; i < SM86_WARP_SIZE; ++i)                         \
        {                                                                \
            TYPE source0 = (TYPE) * source0_current++;                   \
            TYPE source1 = (TYPE) * source1_current++;                   \
            if ((t_mask & 1) && (source0 OP_SYMBOL source1))             \
            {                                                            \
                cmp_result_mask |= 1U << i;                              \
            }                                                            \
            t_mask >>= 1;                                                \
        }                                                                \
    } while (0)

static const uint32_t *sm86_fetch_source1(const sm86_warp_t *POUND_RESTRICT warp,
                                          const sm86_decoded_instruction_t *POUND_RESTRICT
                                                                   instruction,
                                          uint32_t *POUND_RESTRICT temp_buffer);

static const uint32_t *sm86_fetch_source2(const sm86_warp_t *POUND_RESTRICT warp,
                                          const sm86_decoded_instruction_t *POUND_RESTRICT
                                                                   instruction,
                                          uint32_t *POUND_RESTRICT temp_buffer);

static void sm86_execute_bra(const sm86_decoded_instruction_t *POUND_RESTRICT instruction,
                             uint32_t                                         active_threads,
                             uint32_t                                         current_pc,
                             uint32_t                    current_execution_mask,
                             uint32_t                   *next_pc,
                             uint32_t                   *out_execution_mask,
                             sm86_reconvergence_token_t *stack,
                             uint32_t                   *depth,
                             bool                       *branched);

static void sm86_execute_bssy(const sm86_decoded_instruction_t *instruction,
                              uint32_t                          current_pc,
                              uint32_t                          current_execution_mask,
                              sm86_reconvergence_token_t       *stack,
                              uint32_t                         *depth);

static void sm86_execute_bsync(uint32_t                          current_pc,
                               uint32_t                         *out_execution_mask,
                               const sm86_reconvergence_token_t *stack,
                               uint32_t                         *depth);

static void sm86_execute_iadd3(sm86_warp_t                      *warp,
                               const sm86_decoded_instruction_t *inst,
                               uint32_t                          active_threads);

static void sm86_execute_isetp(sm86_warp_t                      *warp,
                               const sm86_decoded_instruction_t *instruction,
                               uint32_t                          active_threads);

static void sm86_execute_exit(uint32_t active_threads, uint32_t *out_execution_mask);

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
        uint32_t       next_pc        = pc + 1;
        bool           branched       = false;

        if (POUND_LIKELY(active_threads != 0))
        {
            switch (inst.opcode)
            {
                case SM86_OPCODE_BRA:
                    sm86_execute_bra(&inst,
                                     active_threads,
                                     pc,
                                     execution_mask,
                                     &next_pc,
                                     &execution_mask,
                                     warp->reconvergence_stack,
                                     &reconvergence_depth,
                                     &branched);
                    break;
                case SM86_OPCODE_BSSY:
                    sm86_execute_bssy(
                        &inst, pc, execution_mask, warp->reconvergence_stack, &reconvergence_depth);
                    break;
                case SM86_OPCODE_BSYNC:
                    sm86_execute_bsync(
                        pc, &execution_mask, warp->reconvergence_stack, &reconvergence_depth);
                case SM86_OPCODE_IADD3:;
                    sm86_execute_iadd3(warp, &inst, active_threads);
                    break;
                case SM86_OPCODE_ISETP:
                    sm86_execute_isetp(warp, &inst, active_threads);
                    break;
                case SM86_OPCODE_EXIT:
                case SM86_OPCODE_KILL:
                    sm86_execute_exit(active_threads, &execution_mask);
                    break;
                default:
                    break;
            }
        }

        pc = next_pc;

        if (POUND_UNLIKELY(true == branched || 0 == execution_mask))
        {
            instructions_cursor = &instructions[pc];
        }
        else
        {
            ++instructions_cursor;
        }

        ++cycles;
    }

    warp->pc                  = pc;
    warp->execution_mask      = execution_mask;
    warp->reconvergence_depth = reconvergence_depth;
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

POUND_HOT static void
sm86_execute_bra(const sm86_decoded_instruction_t *POUND_RESTRICT instruction,
                 const uint32_t                                   active_threads,
                 const uint32_t                                   current_pc,
                 const uint32_t                                   current_execution_mask,
                 uint32_t *POUND_RESTRICT                         next_pc,
                 uint32_t *POUND_RESTRICT                         out_execution_mask,
                 sm86_reconvergence_token_t *POUND_RESTRICT       stack,
                 uint32_t *POUND_RESTRICT                         depth,
                 bool *POUND_RESTRICT                             branched)
{
    const uint32_t target_pc      = current_pc + (uint32_t)instruction->payload.immediate_value;
    const uint32_t taken_mask     = active_threads;
    const uint32_t not_taken_mask = current_execution_mask & ~taken_mask;

    if (taken_mask != 0 && not_taken_mask != 0)
    {
        // Push the fallthrough path to the stack.
        const uint32_t d     = *depth;
        stack[d].pc          = current_pc + 1;
        stack[d].active_mask = not_taken_mask;
        stack[d].type        = SM86_TOKEN_DIVERGENCE;
        *depth               = d + 1;
        *out_execution_mask  = taken_mask;
        *next_pc             = target_pc;
        *branched            = true;
    }
    else if (0 == not_taken_mask)
    {
        // All active threads take the branch.
        *next_pc  = target_pc;
        *branched = true;
    }
    else
    {
    }
}

POUND_HOT static void
sm86_execute_bssy(const sm86_decoded_instruction_t *POUND_RESTRICT instruction,
                  const uint32_t                                   current_pc,
                  const uint32_t                                   current_execution_mask,
                  sm86_reconvergence_token_t *POUND_RESTRICT       stack,
                  uint32_t *POUND_RESTRICT                         depth)
{
    const uint32_t target_pc = current_pc + (uint32_t)instruction->payload.immediate_value;
    const uint32_t d         = *depth;
    stack[d].pc              = target_pc;
    stack[d].active_mask     = current_execution_mask;
    stack[d].type            = SM86_TOKEN_SYNC;
    *depth                   = d + 1;
}

POUND_HOT void
sm86_execute_bsync(const uint32_t                                   current_pc,
                   uint32_t *POUND_RESTRICT                         out_execution_mask,
                   const sm86_reconvergence_token_t *POUND_RESTRICT stack,
                   uint32_t *POUND_RESTRICT                         depth)
{
    uint32_t d = *depth;

    if (POUND_LIKELY(d > 0 && SM86_TOKEN_SYNC == stack[d - 1].type
                     && stack[d - 1].pc == current_pc))
    {
        // We are at the LAST divergent path to reach the sync point.
        *out_execution_mask = stack[d - 1].active_mask;
        *depth              = d - 1;
    }
    else
    {
        // We are an EARLY divergent path. Yield execution by wiping the active mask.
        *out_execution_mask = 0;
    }
}

POUND_HOT static void
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

POUND_HOT void
sm86_execute_isetp(sm86_warp_t *POUND_RESTRICT                      warp,
                   const sm86_decoded_instruction_t *POUND_RESTRICT instruction,
                   uint32_t                                         active_threads)
{
    POUND_ALIGNED(64) uint32_t     temp_source1[SM86_WARP_SIZE];
    const uint32_t *POUND_RESTRICT source0_cursor = warp->gprs[instruction->source0_register];
    const uint32_t *POUND_RESTRICT source1_cursor
        = sm86_fetch_source1(warp, instruction, temp_source1);
    uint32_t thread_mask     = active_threads;
    uint32_t cmp_result_mask = 0;

    if (0 == instruction->cmp_type) // .U32
    {
        switch (instruction->cmp_operator)
        {
            case 0:
                break; // False
            case 1:
                SM86_EVAL_CMP(uint32_t, <);
                break;
            case 2:
                SM86_EVAL_CMP(uint32_t, ==);
                break;
            case 3:
                SM86_EVAL_CMP(uint32_t, <=);
                break;
            case 4:
                SM86_EVAL_CMP(uint32_t, >);
                break;
            case 5:
                SM86_EVAL_CMP(uint32_t, !=);
                break;
            case 6:
                SM86_EVAL_CMP(uint32_t, >=);
                break;
            case 7:
                cmp_result_mask = thread_mask;
                break; // True
            default:
                break;
        }
    }
    else // .I32
    {
        switch (instruction->cmp_operator)
        {
            case 0:
                break; // False
            case 1:
                SM86_EVAL_CMP(int32_t, <);
                break;
            case 2:
                SM86_EVAL_CMP(int32_t, ==);
                break;
            case 3:
                SM86_EVAL_CMP(int32_t, <=);
                break;
            case 4:
                SM86_EVAL_CMP(int32_t, >);
                break;
            case 5:
                SM86_EVAL_CMP(int32_t, !=);
                break;
            case 6:
                SM86_EVAL_CMP(int32_t, >=);
                break;
            case 7:
                cmp_result_mask = thread_mask;
                break; // True
            default:
                break;
        }
    }

    uint32_t accumulator_mask = warp->predicates[instruction->accumulator_predicate];

    if (instruction->accumulator_predicate_not)
    {
        accumulator_mask = ~accumulator_mask;
    }

    uint32_t final_mask = 0;

    switch (instruction->bool_operator)
    {
        case 0:
            final_mask = cmp_result_mask & accumulator_mask;
            break; // AND
        case 1:
            final_mask = cmp_result_mask | accumulator_mask;
            break; // OR
        case 2:
            final_mask = cmp_result_mask ^ accumulator_mask;
            break; // XOR
        default:
            break;
    }

    uint32_t old_destination = warp->predicates[instruction->destination_register];
    warp->predicates[instruction->destination_register]
        = (old_destination & ~active_threads) | (final_mask & active_threads);
}

POUND_HOT static void
sm86_execute_exit(const uint32_t active_threads, uint32_t *POUND_RESTRICT out_execution_mask)
{
    *out_execution_mask &= ~active_threads;
}

/*** end of file ***/