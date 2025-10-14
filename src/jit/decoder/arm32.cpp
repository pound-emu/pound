#include "arm32.h"
#include <string.h>
#include "common/passert.h"

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::decoder
{
/* Increase value as more instructions get implemented */
#define INSTRUCTION_ARRAY_CAPACITY 4
arm32_decoder_t g_arm32_decoder = {};

/*
 * ============================================================================
 *                              Foward Declarations
 * ============================================================================
 */
void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, const char* bitstring, arm32_handler_fn handler);
void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction);

void arm32_parse_bitstring(const char* bitstring, uint32_t* mask, uint32_t* expected);
void arm32_grow_instructions_array(arm32_decoder_t* decoder, size_t new_capacity);

/*
 * ============================================================================
 *                              Public Functions
 * ============================================================================
 */
void arm32_init(pound::host::memory::arena_t allocator, arm32_decoder_t* decoder)
{
    PVM_ASSERT(nullptr != decoder);
    PVM_ASSERT(nullptr != allocator.data);

    (void)memset(decoder, 0, sizeof(arm32_decoder_t));
    decoder->allocator = allocator;

    /* Setup Instructions array.*/
    size_t instructions_array_size = INSTRUCTION_ARRAY_CAPACITY * sizeof(arm32_instruction_info_t);
    PVM_ASSERT(instructions_array_size <= decoder->allocator.capacity);
    LOG_TRACE("Growing instructions array to %d bytes", instructions_array_size);

    void* new_ptr = pound::host::memory::arena_allocate(&decoder->allocator, instructions_array_size);
    PVM_ASSERT(nullptr != new_ptr);

    decoder->instructions = (arm32_instruction_info_t*)new_ptr;
    decoder->instruction_capacity = INSTRUCTION_ARRAY_CAPACITY;

    /* Add all Arm32 instructions */
#define INST(fn, name, bitstring) arm32_add_instruction(decoder, name, bitstring, &arm32_##fn##_handler);
#include "./arm32.inc"
#undef INST
}

void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, const char* bitstring, arm32_handler_fn handler)
{
    PVM_ASSERT(nullptr != decoder);
    PVM_ASSERT(nullptr != decoder->allocator.data);
    PVM_ASSERT(nullptr != name);
    PVM_ASSERT(nullptr != bitstring);
    PVM_ASSERT(decoder->instruction_count < decoder->instruction_capacity);

    LOG_TRACE("Adding '%s' instruction to lookup table.", name);
    arm32_opcode_t mask = 0;
    arm32_opcode_t expected = 0;
    arm32_parse_bitstring(bitstring, &mask, &expected);

    LOG_TRACE("Mask: %x", mask);
    LOG_TRACE("Expected: %x", expected);

    arm32_instruction_info_t* info = &decoder->instructions[decoder->instruction_count];
    info->name = name;
    info->mask = mask;
    info->expected = expected;
    info->handler = handler;

    /* Calculate priority based on number of fixed bits. */
    info->priority = 0;
    for (int i = 0; i < 32; ++i)
    {
        if ((mask >> i) & 1)
        {
            ++info->priority;
        }
    }

    ++decoder->instruction_count;
    arm32_log_instruction_info(info);

    /* TODO(GloriousTacoo:jit): Add instruction to lookup table. */
}
void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction) {}

/*
 * ============================================================================
 *                          Private Functions
 * ============================================================================
 */

void arm32_parse_bitstring(const char* bitstring, uint32_t* mask, uint32_t* expected)
{
    PVM_ASSERT(nullptr != bitstring);
    PVM_ASSERT(nullptr != mask);
    PVM_ASSERT(nullptr != expected);
    PVM_ASSERT(32 == strlen(bitstring));

    *mask = 0;
    *expected = 0;
    uint8_t instruction_size_bits = 32;
    for (int i = 0; (i < instruction_size_bits) && (bitstring[i] != '\0'); ++i)
    {
        uint32_t bit_position = 31 - i;
        switch (bitstring[i])
        {
            case '0':
                *mask |= (1U << bit_position);
                break;
            case '1':
                *mask |= (1U << bit_position);
                *expected |= (1U << bit_position);
                break;
            case 'c':
            case 'n':
            case 'd':
            case 'r':
            case 'v':
            case 's':
            case 'S':
                break;
            default:
                PVM_ASSERT_MSG(false, "Invalid bitstring character: %c", bitstring[i]);
        }
    }
}
void arm32_log_instruction_info(const arm32_instruction_info_t* info)
{
    if (info == nullptr)
    {
        LOG_TRACE("Attempted to log a null instruction info pointer!");
        return;
    }

    LOG_TRACE("========================================");
    LOG_TRACE("Instruction Registered: %s", info->name);
    LOG_TRACE("Mask:      0x%08X", info->mask);
    LOG_TRACE("Expected:  0x%08X", info->expected);
    LOG_TRACE("Priority:  %d", info->priority);
    LOG_TRACE("========================================");
}
}  // namespace pound::jit::decoder
