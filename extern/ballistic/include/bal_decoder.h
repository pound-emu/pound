//! Decodes raw 32-bit ARM64 instruction words into static metadata.

#ifndef BAL_DECODER_H
#define BAL_DECODER_H

#include "bal_attributes.h"
#include "bal_types.h"
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define BAL_OPERANDS_SIZE     5
#define BAL_OPERAND_BIT_WIDTH 5

    /// The type of instruction operand.
    typedef enum
    {
        BAL_OPERAND_TYPE_NONE         = 0,
        BAL_OPERAND_TYPE_REGISTER_32  = 1,
        BAL_OPERAND_TYPE_REGISTER_64  = 2,
        BAL_OPERAND_TYPE_REGISTER_128 = 3,
        BAL_OPERAND_TYPE_IMMEDIATE    = 4,
        BAL_OPERAND_TYPE_CONDITION    = 5,
    } bal_decoder_operand_type_t;

    /// Descriptor for a single operand.
    typedef struct
    {
        /// Operand type. See [`bal_decoder_operand_type_t`].
        uint16_t type : 5;

        /// Bit position in the instruction.
        uint16_t bit_position : 6;

        /// Bit width of the field.
        uint16_t bit_width : BAL_OPERAND_BIT_WIDTH;
    } bal_decoder_operand_t;

    static_assert(2 == sizeof(bal_decoder_operand_t), "Expected operand struct to be 2 bytes.");
    static_assert(5 == BAL_OPERAND_BIT_WIDTH,
                  "Operand bit width must be less than 32 to prevent shift overflow.");

    /// Represents static metadata associated with a specific ARM instruction.
    BAL_ALIGNED(32) typedef struct
    {
        /// The instruction mnemonic.
        const char *name;

        /// A bitmask indicating which bits in the instruction word are
        /// significant for decoding.
        ///
        /// A value of `1` indicates a fixed bit used for identification,
        /// while `0` indicates a variable field (e.g, imm, shamt, Rn).
        uint32_t mask;

        /// The expected pattern of the instruction after applying the `mask`.
        /// `(instruction & mask) == expected`.
        uint32_t expected;

        /// The IR opcode equivalent to this instruction's mnemonic.
        bal_opcode_t ir_opcode;

        /// Descriptors for up to 4 operands.
        bal_decoder_operand_t operands[BAL_OPERANDS_SIZE];

        uint16_t pad;
    } bal_decoder_instruction_metadata_t;

    static_assert(32 == sizeof(bal_decoder_instruction_metadata_t),
                  "Expected decoder metadata struct to be 32 bytes.");

    /// Decodes a raw ARM64 instruction.
    ///
    /// Returns a pointer to [`bal_decoder_instruction_metadata_t`] describing
    /// the instruction if a match is found, or `NULL` if the instruction is
    /// invalid.
    ///
    /// # Safety
    ///
    /// The pointer refers to static readonly memory. It is valid for the
    /// lifetime of the program and must not be freed.
    ///
    /// # Examples
    ///
    /// ```c
    /// #include "bal_decoder.h"
    /// #include <assert.h>
    /// #include <string.h>
    ///
    /// // RET X30.
    /// uint32_t instruction = 0xD65F03C0;
    ///
    /// const bal_decoder_instruction_metadata_t* metadata = bal_decode_arm64(instruction);
    /// assert(strcmp(metadata->name, "RET") == 0);
    /// ```
    BAL_HOT const bal_decoder_instruction_metadata_t *bal_decode_arm64(uint32_t instruction);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // BAL_DECODER_H
