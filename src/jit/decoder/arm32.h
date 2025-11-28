/**
 * @file arm32.h
 * @brief A32 Instruction Decoder Interface.
 *
 * @details
 * This module provides the interface for decoding 32-bit ARM instructions
 * into internal metadata structures.
 *
 * @note
 * While the decoding lookup is thread-safe after initialization, the
 * initialization phase itself is NOT thread-safe.
 */

#ifndef POUND_JIT_DECODER_ARM32_H
#define POUND_JIT_DECODER_ARM32_H

#include <stdint.h>

namespace pound::jit::decoder {

/*! @brief Represents static metadata associated with a specific ARM32 instruction. */
typedef struct
{
    /*! @brief The instruction mnemonic (e.g., "ADD", "LDR"). */
    const char *name;

    /*!
     * @brief The bitmask indicating which bits in the instruction word are significant.
     * @details 1 = significant bit, 0 = variable field (register, immediate, etc.).
     */
    uint32_t    mask;

    /*!
     * @brief The expected value of the instruction after applying the mask.
     * @details (instruction & mask) == expected.
     */
    uint32_t    expected;
} arm32_instruction_info_t;

/*!
 * @brief Initializes the ARM32 decoder lookup tables.
 *
 * @details
 * Populates the internal hash table used for O(1) instruction decoding.
 *
 * @pre  This function must be called during the application startup phase.
 * @post The internal global decoder state is initialized and ready for use.
 *
 * @warning *!Thread Safety*!: Unsafe. This function modifies global state without locking.
 */
void arm32_init(void);

/*!
 * @brief Decodes a raw 32-bit ARM instruction.
 *
 * @details
 * Performs a hash lookup on the provided instruction word to find a matching
 * definition.
 *
 * @param[in] instruction The raw 32-bit ARM machine code to decode.
 *
 * @return A pointer to the instruction metadata if a match is found.
 * @return `nullptr` if the instruction is undefined or invalid.
 *
 * @pre  `arm32_init()` must have been called successfully.
 * @post The returned pointer (if not null) points to static read-only memory.
 *
 * @note *!Thread Safety*!: Safe. This function is read-only re-entrant.
 */
const arm32_instruction_info_t *arm32_decode(const uint32_t instruction);

} // namespace pound::jit::decoder

#endif // POUND_JIT_DECODER_ARM32_H
