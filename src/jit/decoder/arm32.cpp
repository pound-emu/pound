#include "arm32.h"
#include <string.h>
#include "common/passert.h"

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::decoder
{
/* Increase value as more instructions get implemented */
#define INSTRUCTION_ARRAY_CAPACITY 4

#define HASH_TABLE_INVALID_INDEX 0xFFFF

/*
 * ============================================================================
 *                              Foward Declarations
 * ============================================================================
 */
void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, const char* bitstring, arm32_handler_fn handler);
void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction);
void arm32_SUB_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction);

void arm32_parse_bitstring(const char* bitstring, uint32_t* mask, uint32_t* expected);
void arm32_grow_instructions_array(arm32_decoder_t* decoder, size_t new_capacity);
uint32_t arm32_generate_hash_seed(uint32_t mask, uint32_t expected);

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
#define INST(fn, name, bitstring) arm32_add_instruction(decoder, name, bitstring, &arm32_##fn##_handler);
#include "./arm32.inc"
#undef INST

    LOG_TRACE("Initializing perfect hash parameters for %zu instructions", decoder->instruction_count);

    /* Start with table size as next power of 2 >= count * 2 */
    uint16_t table_size = 1U;
    while (table_size < decoder->instruction_count * 2)
    {
        table_size <<= 1U;
    }

    uint16_t mask = table_size - 1;
    size_t slots_used_size = table_size * sizeof(bool);
    bool* slots_used = (bool*)pound::host::memory::arena_allocate(&decoder->allocator, slots_used_size);
    PVM_ASSERT(nullptr != slots_used);
    LOG_TRACE("Allocated %d bytes to hash slots used array", slots_used_size);

    uint16_t shift = 0;
    bool perfect_hash_generated = false;
    for (; shift < 16; ++shift)
    {
        bool collision_free = true;
        (void)memset(slots_used, false, slots_used_size);

        uint16_t hash;
        for (size_t i = 0; i < decoder->instruction_count; ++i)
        {
            uint32_t hash_seed =
                arm32_generate_hash_seed(decoder->instructions[i].mask, decoder->instructions[i].expected);

            /*
             * Now we need to convert the 32-bit hash seed into an index within our hash table.
             * We right shift by `shift` bits. Different shift values produce different hash distributions. We found
             * the shift value that ensures no two instructions map to the same hash table slot.
             */
            hash = ((hash_seed >> shift) & mask);
            if (true == slots_used[hash])
            {
                LOG_TRACE("Instruction %s: Collision detected at slot %u for shift %u", decoder->instructions[i].name,
                          hash, shift);
                collision_free = false;
                break;
            }
            slots_used[hash] = true;
            LOG_TRACE("Instruction %s: Collision-free hash %u found for shift %u", decoder->instructions[i].name, hash,
                      shift);
        }
        if (true == collision_free)
        {
            perfect_hash_generated = true;
            break;
        }
    }

    PVM_ASSERT_MSG(true == perfect_hash_generated, "Failed to generate perfect hash - no collision-free hash found");
    LOG_TRACE("Perfect hash parameters: shift=%u, mask=0x%04x, table_size=%u", shift, mask, table_size);

    size_t hash_table_size = table_size * sizeof(uint16_t);
    uint16_t* hash_table = (uint16_t*)pound::host::memory::arena_allocate(&decoder->allocator, hash_table_size);
    PVM_ASSERT(nullptr != hash_table);
    LOG_TRACE("Allocated %zu bytes to hash table", hash_table_size);

    /* Initialize hash table with invalid indices */
    for (size_t i = 0; i < table_size; ++i)
    {
        hash_table[i] = HASH_TABLE_INVALID_INDEX;
    }

    for (size_t i = 0; i < decoder->instruction_count; ++i)
    {
        uint32_t hash_seed = arm32_generate_hash_seed(decoder->instructions[i].mask, decoder->instructions[i].expected);
        uint32_t hash = (hash_seed >> shift) & mask;
        PVM_ASSERT_MSG(HASH_TABLE_INVALID_INDEX == hash_table[hash], "Hash collision detected");

        PVM_ASSERT(i < 0xFFFF);
        hash_table[hash] = (uint16_t)i;

        LOG_TRACE("Instruction '%s' hashed to slot %u", decoder->instructions[i].name, hash);
    }

    decoder->perfect_hash.hash_shift = shift;
    decoder->perfect_hash.hash_mask = mask;
    decoder->perfect_hash.table_size = table_size;
    decoder->perfect_hash.hash_table = hash_table;
}

/*
 * ============================================================================
 *                          Private Functions
 * ============================================================================
 */

void arm32_add_instruction(arm32_decoder_t* decoder, const char* name, const char* bitstring, arm32_handler_fn handler)
{
    PVM_ASSERT(nullptr != decoder);
    PVM_ASSERT(nullptr != decoder->allocator.data);
    PVM_ASSERT(nullptr != name);
    PVM_ASSERT(nullptr != bitstring);
    PVM_ASSERT(decoder->instruction_count < decoder->instruction_capacity);

    arm32_opcode_t mask = 0;
    arm32_opcode_t expected = 0;
    arm32_parse_bitstring(bitstring, &mask, &expected);

    arm32_instruction_info_t* info = &decoder->instructions[decoder->instruction_count];
    info->name = name;
    info->mask = mask;
    info->expected = expected;
    info->handler = handler;

    /* Calculate priority based on number of fixed bits. */
    info->priority = (uint8_t)__builtin_popcount(mask);
    ++decoder->instruction_count;

    LOG_TRACE("Instruction Registered: %s", info->name);
    LOG_TRACE("Mask:      0x%08X", info->mask);
    LOG_TRACE("Expected:  0x%08X", info->expected);
    LOG_TRACE("Priority:  %d", info->priority);
}

void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction) {}
void arm32_SUB_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction) {}

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
uint32_t arm32_generate_hash_seed(uint32_t mask, uint32_t expected)
{
    /* 
             * XOR combines the mask (which bits matter) and expected value (what those btis should be)
             * into a single 32-bit value that uniquely represents this instruction pattern.
             * Example: ADD instruction might have mask=0x0FF00000, expected=0x00200000
             *          value = 0x0FF00000 ^ 0x00200000 = 0x0FD00000;
             */
    uint32_t value = mask ^ expected;

    /*
             * First bit mixing round - break up patterns in the value.
             * We need to eliminate patterns (like trailing zeroes) that cause collisions.
             * We do this by mixing the high bits (16-31) with the low bits (0-15).
             *
             * We then multiply a magic number to further scrambles the bits to ensure good distribution.
             */
    value = ((value >> 16) ^ value) * 0x45d9f3b;

    /*
             * Second bit mixing round to ensire thorough mixing
             */
    value = ((value >> 16) ^ value) * 0x45d9f3b;

    /*
             * Final mixing. The result is our hash seed - a 32-bit number that uniquely represents this instruction.
             */
    value = (value >> 16) ^ value;
    return value;
}
}  // namespace pound::jit::decoder
