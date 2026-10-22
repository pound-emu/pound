/** @file bal_types.h
 *
 * @brief Defines types used by Ballistic.
 */

#ifndef BALLISTIC_TYPES_H
#define BALLISTIC_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef uint64_t bal_guest_address_t;
    typedef uint64_t bal_instruction_t;
    typedef uint64_t bal_constant_t;
    typedef uint16_t bal_instruction_count_t;
    typedef uint16_t bal_constant_count_t;
    typedef uint8_t  bal_bit_width_t;

    typedef enum
    {
        /// This opcode represents a volatile load of a guest register.
        OPCODE_GET_REGISTER,
        OPCODE_MOV,
        OPCODE_ADD,
        OPCODE_SUB,
        OPCODE_MUL,
        OPCODE_DIV,
        OPCODE_AND,
        OPCODE_XOR,
        OPCODE_OR_NOT,
        OPCODE_SHIFT,
        OPCODE_LOAD,
        OPCODE_STORE,
        OPCODE_JUMP,
        OPCODE_CALL_HOST,
        OPCODE_BRANCH_ZERO,
        OPCODE_BRANCH_NOT_ZERO,
        OPCODE_BRANCH_CONDITIONAL,
        OPCODE_TEST_BIT_ZERO,
        OPCODE_CMP,
        OPCODE_CONDITIONAL_SELECT,
        OPCODE_IF,
        OPCODE_LOOP,
        OPCODE_MERGE,
        OPCODE_YIELD,
        OPCODE_BREAK,
        OPCODE_CONTINUE,
        OPCODE_RETURN,
        OPCODE_TRAP,
        OPCODE_BLOCK_ARG,
        OPCODE_ARG_EXTENSION,
        OPCODE_VOID,
        OPCODE_NOP,
        OPCODE_GET_FLAG_N,
        OPCODE_GET_FLAG_Z,
        OPCODE_GET_FLAG_C,
        OPCODE_GET_FLAG_V,
        OPCODE_SET_FLAG_N,
        OPCODE_SET_FLAG_Z,
        OPCODE_SET_FLAG_C,
        OPCODE_SET_FLAG_V,
        OPCODE_ENUM_END = 0x7FF
    } bal_opcode_t;

    typedef enum
    {
        /// The register index for X0.
        BAL_REGISTER_X0 = 0,

        /// The register index for X1.
        BAL_REGISTER_X1 = 1,

        /// The register index for X2.
        BAL_REGISTER_X2 = 2,

        /// The register index for X3.
        BAL_REGISTER_X3 = 3,

        /// The register index for X4.
        BAL_REGISTER_X4 = 4,

        /// The register index for X5.
        BAL_REGISTER_X5 = 5,

        /// The register index for X6.
        BAL_REGISTER_X6 = 6,

        /// The register index for X7.
        BAL_REGISTER_X7 = 7,

        /// The register index for X8.
        BAL_REGISTER_X8 = 8,

        /// The register index for X9.
        BAL_REGISTER_X9 = 9,

        /// The register index for 10.
        BAL_REGISTER_X10 = 10,

        /// The register index for 11.
        BAL_REGISTER_X11 = 11,

        /// The register index for 12.
        BAL_REGISTER_X12 = 12,

        /// The register index for 13.
        BAL_REGISTER_X13 = 13,

        /// The register index for 14.
        BAL_REGISTER_X14 = 14,

        /// The register index for 15.
        BAL_REGISTER_X15 = 15,

        /// The register index for 16.
        BAL_REGISTER_X16 = 16,

        /// The register index for 17.
        BAL_REGISTER_X17 = 17,

        /// The register index for 18.
        BAL_REGISTER_X18 = 18,

        /// The register index for 19.
        BAL_REGISTER_X19 = 19,

        /// The register index for 20.
        BAL_REGISTER_X20 = 20,

        /// The register index for 21.
        BAL_REGISTER_X21 = 21,

        /// The register index for 22.
        BAL_REGISTER_X22 = 22,

        /// The register index for 23.
        BAL_REGISTER_X23 = 23,

        /// The register index for 24.
        BAL_REGISTER_X24 = 24,

        /// The register index for 25.
        BAL_REGISTER_X25 = 25,

        /// The register index for 26.
        BAL_REGISTER_X26 = 26,

        /// The register index for 27.
        BAL_REGISTER_X27 = 27,

        /// The register index for 28.
        BAL_REGISTER_X28 = 28,

        /// The register index for 29 (Frame Pointer).
        BAL_REGISTER_X29 = 29,

        /// The register index for 30 (Link Register).
        BAL_REGISTER_X30 = 30,

        /// The register index for the Zero Register (XZR)/Stack Pointer (SP).
        BAL_REGISTER_X31 = 31,
    } bal_register_index_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BALLISTIC_TYPES_H */

/*** end of file ***/
