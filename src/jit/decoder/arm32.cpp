#include "arm32.h"
#include "common/passert.h"
#include <string.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::decoder {
#define INSTRUCTION_ARRAY_CAPACITY 261
#define INSTRUCTION_BITSTRING_LENGTH 32

/*
 * ============================================================================
 *                              Foward Declarations
 * ============================================================================
 */
static void arm32_add_instruction(arm32_decoder_t *p_decoder,
                                  const char      *p_name,
                                  const char      *p_bitstring);
static void arm32_parse_bitstring(const char *p_bitstring,
                                  uint32_t   *p_mask,
                                  uint32_t   *p_expected);

/*
 * ============================================================================
 *                              Public Functions
 * ============================================================================
 */
void
arm32_init (pound::host::memory::arena_t allocator, arm32_decoder_t *p_decoder)
{
    PVM_ASSERT(nullptr != p_decoder);
    PVM_ASSERT(nullptr != allocator.data);

    (void)memset(p_decoder, 0, sizeof(arm32_decoder_t));
    p_decoder->allocator = allocator;

    /* Setup Instructions array.*/
    size_t instructions_array_size
        = INSTRUCTION_ARRAY_CAPACITY * sizeof(arm32_instruction_info_t);
    PVM_ASSERT(instructions_array_size <= p_decoder->allocator.capacity);
    LOG_TRACE("Allocated %d bytes to instructions array",
              instructions_array_size);

    void *p_memory = pound::host::memory::arena_allocate(
        &p_decoder->allocator, instructions_array_size);
    PVM_ASSERT(nullptr != p_memory);

    p_decoder->p_instructions       = (arm32_instruction_info_t *)p_memory;
    p_decoder->instruction_capacity = INSTRUCTION_ARRAY_CAPACITY;

    /* Add all Arm32 instructions */
#define INST(fn, name, bitstring) \
    arm32_add_instruction(p_decoder, name, bitstring);
#include "./arm32.inc"
#undef INST
}

arm32_instruction_info_t *
arm32_decode (arm32_decoder_t *p_decoder, uint32_t instruction)
{
    for (size_t i = 0; i < p_decoder->instruction_count; ++i)
    {
        arm32_instruction_info_t *p_info = &p_decoder->p_instructions[i];
        if ((instruction & p_info->mask) == p_info->expected)
        {
            LOG_TRACE("Instruction found for 0x%08X: %s",
                      instruction,
                      p_info->p_name);
            return p_info;
        }
    }
    PVM_ASSERT_MSG(false, "No instruction found for 0x%08X", instruction);
}

/*
 * ============================================================================
 *                          Private Functions
 * ============================================================================
 */

static void
arm32_add_instruction (arm32_decoder_t *p_decoder,
                       const char      *p_name,
                       const char      *p_bitstring)
{
    PVM_ASSERT(nullptr != p_decoder);
    PVM_ASSERT(nullptr != p_decoder->allocator.data);
    PVM_ASSERT(nullptr != p_name);
    PVM_ASSERT(nullptr != p_bitstring);
    PVM_ASSERT(p_decoder->instruction_count < p_decoder->instruction_capacity);

    uint32_t mask     = 0;
    uint32_t expected = 0;
    arm32_parse_bitstring(p_bitstring, &mask, &expected);

    arm32_instruction_info_t *p_info
        = &p_decoder->p_instructions[p_decoder->instruction_count];
    PVM_ASSERT(nullptr != p_info);
    p_info->p_name   = p_name;
    p_info->mask     = mask;
    p_info->expected = expected;

    ++p_decoder->instruction_count;

    LOG_TRACE("Instruction Registered: %s", p_info->p_name);
    LOG_TRACE("Mask:      0x%08X", p_info->mask);
    LOG_TRACE("Expected:  0x%08X", p_info->expected);
}

static void
arm32_parse_bitstring (const char *p_bitstring,
                       uint32_t   *p_mask,
                       uint32_t   *p_expected)
{
    PVM_ASSERT(nullptr != p_bitstring);
    PVM_ASSERT(nullptr != p_mask);
    PVM_ASSERT(nullptr != p_expected);
    PVM_ASSERT(INSTRUCTION_BITSTRING_LENGTH == strlen(p_bitstring));

    *p_mask                       = 0;
    *p_expected                   = 0;
    uint8_t instruction_size_bits = 32;
    for (unsigned int i = 0;
         (i < instruction_size_bits) && (p_bitstring[i] != '\0');
         ++i)
    {
        uint32_t bit_position = 31 - i;
        switch (p_bitstring[i])
        {
            case '0':
                *p_mask |= (1U << bit_position);
                break;
            case '1':
                *p_mask |= (1U << bit_position);
                *p_expected |= (1U << bit_position);
            default:
                break;
        }
    }
}
} // namespace pound::jit::p_decoder
