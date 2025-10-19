#include "arm32.h"
#include <string.h>
#include "common/passert.h"

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::decoder
{
#define INSTRUCTION_ARRAY_CAPACITY 261

#define HASH_TABLE_INVALID_INDEX 0xFFFF

/*
 * ============================================================================
 *                              Foward Declarations
 * ============================================================================
 */
void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, const char* bitstring);

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
    LOG_TRACE("Allocated %d bytes to instructions array", instructions_array_size);

    void* new_ptr = pound::host::memory::arena_allocate(&decoder->allocator, instructions_array_size);
    PVM_ASSERT(nullptr != new_ptr);

    decoder->instructions = (arm32_instruction_info_t*)new_ptr;
    decoder->instruction_capacity = INSTRUCTION_ARRAY_CAPACITY;

    /* Add all Arm32 instructions */
#define INST(fn, name, bitstring) arm32_add_instruction(decoder, name, bitstring);
#include "./arm32.inc"
#undef INST
}

arm32_instruction_info_t* arm32_decode(arm32_decoder_t* decoder, uint32_t instruction)
{
    for (size_t i = 0; i < decoder->instruction_count; ++i)
    {
        arm32_instruction_info_t* info = &decoder->instructions[i];
        if ((instruction & info->mask) == info->expected)
        {
            LOG_TRACE("Instruction found for 0x%08X: %s", instruction, info->name);
            return info;
        }
    }
    PVM_ASSERT_MSG(false, "No instruction found for 0x%08X", instruction);
}

/*
 * ============================================================================
 *                          Private Functions
 * ============================================================================
 */

void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, const char* bitstring)
{
    PVM_ASSERT(nullptr != decoder);
    PVM_ASSERT(nullptr != decoder->allocator.data);
    PVM_ASSERT(nullptr != name);
    PVM_ASSERT(nullptr != bitstring);
    PVM_ASSERT(decoder->instruction_count < decoder->instruction_capacity);

    uint32_t mask = 0;
    uint32_t expected = 0;
    arm32_parse_bitstring(bitstring, &mask, &expected);

    arm32_instruction_info_t* info = &decoder->instructions[decoder->instruction_count];
    PVM_ASSERT(nullptr != info);
    info->name = name;
    info->mask = mask;
    info->expected = expected;

    ++decoder->instruction_count;

    LOG_TRACE("Instruction Registered: %s", info->name);
    LOG_TRACE("Mask:      0x%08X", info->mask);
    LOG_TRACE("Expected:  0x%08X", info->expected);
}

void arm32_parse_bitstring(const char* bitstring, uint32_t* mask, uint32_t* expected)
{
    PVM_ASSERT(nullptr != bitstring);
    PVM_ASSERT(nullptr != mask);
    PVM_ASSERT(nullptr != expected);
    PVM_ASSERT(32 == strlen(bitstring));

    *mask = 0;
    *expected = 0;
    uint8_t instruction_size_bits = 32;
    for (unsigned int i = 0; (i < instruction_size_bits) && (bitstring[i] != '\0'); ++i)
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
            default:
                break;
        }
    }
}
}  // namespace pound::jit::decoder
