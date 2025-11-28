#include "arm32.h"
#include "common/passert.h"
#include <string.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::decoder {

/*! @brief Maximum number of instructions allowed in a single hash bucket. */
#define LOOKUP_TABLE_MAX_BUCKET_SIZE 8

/*! @brief Size of the lookup table (12-bit index). */
#define LOOKUP_TABLE_INDEX_MASK 0xFFF

/*! @brief Expected length of the bitstring representation in the .inc file. */
#define INSTRUCTION_BITSTRING_LENGTH 32

/*!
 * @brief   A bucket within the decoding hash table.
 * @details Stores a list of instructions that collide on the same hash index.
 */
typedef struct
{
    /*! @brief Array of pointers to instruction definitions. */
    const arm32_instruction_info_t *instructions[LOOKUP_TABLE_MAX_BUCKET_SIZE];

    /*! @brief Current number of instructions in this bucket. */
    size_t count;
} decode_bucket_t;

/*! @brief The internal state of the decoder. */
typedef struct
{
    /*!
     * @brief   The main lookup table.
     * @details Index constructed from bits [27:20] and [7:4] of the
     * instruction.
     */
    decode_bucket_t lookup_table[LOOKUP_TABLE_INDEX_MASK + 1];

    /*! @brief Initialization guard flag. */
    bool is_initialized;
} decoder_t;

/*! @brief Global decoder instance. */
static decoder_t g_decoder = {};

/*
 * ============================================================================
 *                              Compile-Time Functions
 * ============================================================================
 */

/*! @brief Prototype for a non-constexpr function to force build failure. */
void BUILD_ERROR_ARM32_BITSTRING_MUST_BE_32_CHARS(void);

/*!
 * @brief Parses a binary string literal into a bitmask at compile time.
 *
 * @param[in] string The null-terminated string literal.
 * @return           The calculated uint32_t mask.
 */
consteval uint32_t
parse_mask (const char *string)
{
    size_t length = 0;

    for (; string[length] != '\0'; ++length)
    {
    }

    if (length != INSTRUCTION_BITSTRING_LENGTH)
    {
        BUILD_ERROR_ARM32_BITSTRING_MUST_BE_32_CHARS();
    }

    uint32_t mask = 0;
    for (int i = 0; i < 32; ++i)
    {
        const uint32_t bit = 1U << (31 - i);
        if ('0' == string[i] || string[i] == '1')
        {
            mask |= bit;
        }
    }
    return mask;
}

/*!
 * @brief Parses a binary string literal into an expected value at compile time.
 *
 * @param[in] string The null-terminated string literal.
 * @return           The calculated uint32_t expected value.
 */
consteval uint32_t
parse_expected (const char *string)
{
    uint32_t expected = 0;
    for (int i = 0; i < 32; ++i)
    {
        const uint32_t bit = 1U << (31 - i);
        if (string[i] == '1')
        {
            expected |= bit;
        }
    }
    return expected;
}

/*! @brief List of all supported ARM32 instructions. */
static const arm32_instruction_info_t g_instructions[] = {
#define INST(fn, name, bitstring) \
    { name, parse_mask(bitstring), parse_expected(bitstring) },
#include "arm32.inc"
#undef INST
};

/*! @brief The total number of defined instructions. */
#define INSTRUCTION_ARRAY_CAPACITY \
    (sizeof(g_instructions) / sizeof(g_instructions[0]))

/*
 * ============================================================================
 *                              Public Functions
 * ============================================================================
 */

void
arm32_init (void)
{
    PVM_ASSERT_MSG(false == g_decoder.is_initialized,
                   "Decoder already initialized.");

    (void)memset(g_decoder.lookup_table, 0, sizeof(g_decoder.lookup_table));

    // Populate the hash table.
    for (uint32_t i = 0; i <= LOOKUP_TABLE_INDEX_MASK; ++i)
    {
        decode_bucket_t *bucket = &g_decoder.lookup_table[i];

        // Reconstruct the instruction bits that correspond to this hash index.
        // Bits [27:20] and [7:4].
        const uint32_t synthetic_instruction
            = ((i & 0xFF0) << 16) | ((i & 0xF) << 4);

        for (size_t ii = 0; ii < INSTRUCTION_ARRAY_CAPACITY; ++ii)
        {
            const arm32_instruction_info_t *info = &g_instructions[ii];

            /* Mask corresponding to the hash bits: 0x0FF000F0 */
            const uint32_t index_bits_mask = 0x0FF000F0;
            const uint32_t relevant_mask   = index_bits_mask | info->mask;

            if ((synthetic_instruction & relevant_mask)
                == (info->expected & relevant_mask))
            {
                LOG_TRACE("Mapping instruction '%s' to LUT Index 0x%03X",
                          info->name,
                          i);

                if (bucket->count >= LOOKUP_TABLE_MAX_BUCKET_SIZE)
                {
                    PVM_ASSERT_MSG(
                        false,
                        "ARM32 LUT Collision Overflow at index 0x%03X. "
                        "Increase MAX_LUT_BUCKET_SIZE.",
                        i);
                }

                if (bucket->count >= (LOOKUP_TABLE_MAX_BUCKET_SIZE / 2))
                {
                    LOG_WARNING(
                        "High collision density at Index 0x%03X (Count: %zu).",
                        i,
                        bucket->count + 1);
                }
                bucket->instructions[bucket->count] = info;
                ++bucket->count;
            }
        }
    }

    g_decoder.is_initialized = true;
    LOG_INFO("ARM32 Decoder initialized with %zu instructions",
             INSTRUCTION_ARRAY_CAPACITY);
}

const arm32_instruction_info_t *
arm32_decode (const uint32_t instruction)
{
    PVM_ASSERT_MSG(true == g_decoder.is_initialized,
                   "Decoder needs to initialize.");

    /* Extract hash key: Bits [27:20] and [7:4] */
    const uint32_t major = (instruction >> 20) & 0xFF;
    const uint32_t minor = (instruction >> 4) & 0xF;

    const uint16_t index = (uint16_t)((major << 4) | minor);

    const decode_bucket_t *bucket = &g_decoder.lookup_table[index];

    for (size_t i = 0; i < bucket->count; ++i)
    {
        const arm32_instruction_info_t *info = bucket->instructions[i];

        if ((instruction & info->mask) == info->expected)
        {

            return info;
        }
    }

    return nullptr;
}

} // namespace pound::jit::decoder
