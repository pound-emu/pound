/*
 * Reads ARM code and produces the an array of interpreter instructions.
 */

#include "translator.h"
#include "common/passert.h"
#include "common/pmath.h"
#include <stdio.h>

#define LOG_MODULE "jit::interpreter"
#include "common/logging.h"


pvm_jit_interpreter_arm32_translate_result_t
pvm_jit_interpreter_arm32_translate (pvm_host_memory_arena_t *restrict arena,
                                     pvm_jit_interpreter_arm32_block_t *block,
                                     const uint32_t *restrict guest_code,
                                     const size_t max_instructions)
{
    PVM_ASSERT(NULL != arena);
    PVM_ASSERT(NULL != arena->data);
    PVM_ASSERT(NULL != block);
    PVM_ASSERT(NULL != guest_code);

    LOG_TRACE("Starting translation: %p, Max: %zu",
              (const void *)guest_code,
              max_instructions);

    block->instructions = NULL;
    block->count        = 0;

    size_t count_plus_sentinel;
    if (true == safe_add_size_t(max_instructions, 1, &count_plus_sentinel))
    {
        LOG_ERROR("Integer overflow when calculating instruction count");
        return PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OVERFLOW;
    }

    size_t allocation_size;
    if (true
        == safe_multiply_size_t(count_plus_sentinel,
                                sizeof(pvm_jit_interpreter_arm32_instruction_t),
                                &allocation_size))
    {
        LOG_ERROR("Integer overflow whe calculating allocation byte size");
        return PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OVERFLOW;
    }

    size_t required_capacity;
    if (safe_add_size_t(arena->size, allocation_size, &required_capacity))
    {
        LOG_ERROR("Integer overflow when checking arena capacity");
        return PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OVERFLOW;
    }

    if (required_capacity > arena->capacity)
    {
        LOG_ERROR(
            "JIT Translation OOM. Used: %zu, Capacity: %zu, Required: %zu",
            arena->size,
            arena->capacity,
            allocation_size);
        return PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OUT_OF_MEMORY;
    }

    pvm_jit_interpreter_arm32_instruction_t *const instructions
        = pvm_host_memory_arena_allocate(arena, allocation_size);

    if (NULL == instructions)
    {
        LOG_ERROR("Allocating instructions array failed");
        return PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    size_t instruction_count = 0;
    bool   stop_block        = false;
    for (size_t i = 0; i < max_instructions; ++i)
    {
        const uint32_t raw_instruction = guest_code[i];
        const pvm_jit_decoder_arm32_instruction_info_t *info
            = pvm_jit_decoder_arm32_decode(raw_instruction);
        PVM_ASSERT(NULL != info);

        pvm_jit_interpreter_arm32_instruction_t *current_emit
            = &instructions[instruction_count];
        PVM_ASSERT(NULL != current_emit);

        current_emit->raw = raw_instruction;

        if (NULL != info)
        {
            current_emit->opcode = info->opcode;

            /*
             * Optimization: Stop Translation on Branch.
             * This prevents decoding dead code and ensures the interpreter
             * updates the Program Counter (PC) correctly before executing the
             * next block.
             */
            switch (info->opcode)
            {
                case PVM_A32_OP_B:
                case PVM_A32_OP_BL:
                case PVM_A32_OP_BX:
                case PVM_A32_OP_BLX_IMM:
                case PVM_A32_OP_BLX_REG:
                case PVM_A32_OP_BXJ:
                    LOG_TRACE(
                        "End of block detected: Direct Branch (Opcode: %s) at "
                        "index %zu",
                        info->name,
                        i);
                    stop_block = true;
                    break;

                /* ALU operations that write to PC (R15) are also branches */
                case PVM_A32_OP_ADC_IMM:
                case PVM_A32_OP_ADC_REG:
                case PVM_A32_OP_ADC_RSR:
                case PVM_A32_OP_ADD_IMM:
                case PVM_A32_OP_ADD_REG:
                case PVM_A32_OP_ADD_RSR:
                case PVM_A32_OP_AND_IMM:
                case PVM_A32_OP_AND_REG:
                case PVM_A32_OP_AND_RSR:
                case PVM_A32_OP_BIC_IMM:
                case PVM_A32_OP_BIC_REG:
                case PVM_A32_OP_BIC_RSR:
                case PVM_A32_OP_EOR_IMM:
                case PVM_A32_OP_EOR_REG:
                case PVM_A32_OP_EOR_RSR:
                case PVM_A32_OP_MOV_IMM:
                case PVM_A32_OP_MOV_REG:
                case PVM_A32_OP_MOV_RSR:
                case PVM_A32_OP_MVN_IMM:
                case PVM_A32_OP_MVN_REG:
                case PVM_A32_OP_MVN_RSR:
                case PVM_A32_OP_ORR_IMM:
                case PVM_A32_OP_ORR_REG:
                case PVM_A32_OP_ORR_RSR:
                case PVM_A32_OP_RSB_IMM:
                case PVM_A32_OP_RSB_REG:
                case PVM_A32_OP_RSB_RSR:
                case PVM_A32_OP_RSC_IMM:
                case PVM_A32_OP_RSC_REG:
                case PVM_A32_OP_RSC_RSR:
                case PVM_A32_OP_SBC_IMM:
                case PVM_A32_OP_SBC_REG:
                case PVM_A32_OP_SBC_RSR:
                case PVM_A32_OP_SUB_IMM:
                case PVM_A32_OP_SUB_REG:
                case PVM_A32_OP_SUB_RSR: {
                    const uint32_t rd = (raw_instruction >> 12U) & 0xFU;
                    if (15U == rd)
                    {
                        LOG_TRACE(
                            "End of block detected: ALU Branch (Opcode: %s) at "
                            "index %zu",
                            info->name,
                            i);
                        stop_block = true;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else
        {
            /* Decoding failed. Treat as STOP. */
            LOG_WARNING(
                "Decode failed at index %zu: 0x%08X", i, raw_instruction);
            current_emit->opcode = PVM_A32_OP_STOP;
            stop_block           = true;
        }
        instruction_count++;

        if (true == stop_block)
        {
            break;
        }
    }

    /*
     * Always append a STOP instruction at the end of the block.
     * This guarantees the interpreter loop (computed goto) never runs
     * off the end of the buffer into garbage memory.
     */
    instructions[instruction_count].opcode = PVM_A32_OP_STOP;
    instructions[instruction_count].raw    = 0;
    instruction_count++;

    block->instructions = instructions;
    block->count        = instruction_count;

    LOG_TRACE("Translation complete. Generated %zu instructions.", instruction_count);
    return PVM_JIT_INTERPRETER_ARM32_TRANSLATE_SUCCESS;
}
