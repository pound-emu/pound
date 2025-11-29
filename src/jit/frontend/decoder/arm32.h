/**
 * @file arm32.h
 * @brief A32 Instruction Decoder Interface.
 *
 * @details
 * This module provides the interface for decoding 32-bit ARM instructions
 * into internal metadata structures.
 *
 */

#ifndef POUND_JIT_DECODER_ARM32_H
#define POUND_JIT_DECODER_ARM32_H

#include <stdint.h>

/*! @brief Represents static metadata associated with a specific ARM32
 * instruction. */
typedef struct
{
    /*! @brief The instruction mnemonic (e.g., "ADD", "LDR"). */
    const char *name;

    /*!
     * @brief The raw bitstring representation.
     * @details Used during initialization to calculate mask and expected
     * values.
     */
    const char *bitstring;

    /*!
     * @brief The bitmask indicating which bits in the instruction word are
     * significant.
     * @details 1 = significant bit, 0 = variable field (register, immediate,
     * etc.).
     */
    uint32_t mask;

    /*!
     * @brief The expected value of the instruction after applying the mask.
     * @details (instruction & mask) == expected.
     */
    uint32_t expected;
} pvm_jit_decoder_arm32_instruction_info_t;

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
 * @post The returned pointer (if not null) points to static read-only memory.
 */
const pvm_jit_decoder_arm32_instruction_info_t *pvm_jit_decoder_arm32_decode(
    const uint32_t instruction);

#endif // POUND_JIT_DECODER_ARM32_H
