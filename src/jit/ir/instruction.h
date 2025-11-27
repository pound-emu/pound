#ifndef POUND_JIT_IR_INSTRUCTION_H
#define POUND_JIT_IR_INSTRUCTION_H

#include "opcode.h"
#include "value.h"
#include <stddef.h>

namespace pound::jit::ir {
// Maximum number of arguments an IR instruction can have.
#define MAX_IR_ARGS 4

/*!
 * Represents a single instruction in the IR layer.
 *
 * Each instruction node encapsulates an opcode, its arguments, and pointers to
 * form an intrusive double-linked list.
 */
typedef struct instruction_t
{
    // The opcode for this instruction.
    opcode_t opcode;

    // An array of arguments for this instruction.
    value_t args[MAX_IR_ARGS];
} instruction_t;

/*!
 * @brief Gets a pointer to the argument at a specific index.
 *
 * @param instruction   Pointer to the IR instruction.
 * @param arg_index     The index of the argument to retrieve.
 *
 * @return  A constant pointer to the argument at the specified index.
 * @pre     `instruction` must not be NULL
 * @pre     `arg_index` must be less than `MAX_IR_ARGS`.
 */
value_t *instruction_get_arg(instruction_t *instruction, size_t arg_index);

/*!
 * Retrieves a U64 argument from an instruction.
 *
 * @param instruction   Pointer to the IR instruction.
 * @apram arg_index     The index of the argument to retrieve.
 *
 * @return  The U64 value of the argument.
 * @pre     `instruction` must not be NULL.
 * @pre     `arg_index` must be less than `MAX_IR_ARGS`.
 * @pre     The argument at `arg_index` must be of type `IR_TYPE_U64`.
 */
uint64_t instruction_get_arg_u64(instruction_t *instruction, size_t arg_index);

/*!
 * @brief Retrieves a U32 argument from an instruction.
 *
 * @param instruction   Pointer to the IR instruction.
 * @param arg_index     The index of the argument to retrieve.
 *
 * @return  The U32 value of the argument.
 * @pre     `instruction` must not be NULL.
 * @pre     `arg_index` must be less than `MAX_IR_ARGS`.
 * @pre     The argument at `arg_index` must be of type `IR_TYPE_U32`.
 */
uint32_t instruction_get_arg_u32(instruction_t *instruction, size_t arg_index);

/*!
 * Retrives a U8 argument from an instruction.
 *
 * @param instruction   Pointer to the IR instruction.
 * @param arg_index     The index of the argument to retrieve.
 *
 * @return  The U8 value of the argument.
 * @pre     `instruction` must not be NULL.
 * @pre     `arg_index` must be less than `MAX_IR_ARGS`.
 * @pre     The argument at `arg_index` must be of type `IR_TYPE_U8`.
 */
uint8_t instruction_get_arg_u8(instruction_t *instruction, size_t arg_index);

/*!
 * @brief Retrieves a U1 (boolean) argument from an instruction.
 *
 * @param instruction   Pointer to the IR instruction.
 * @param arg_index     The index of the argument to retrieve.
 *
 * @return  The boolean value of the argument.
 * @pre     `instruction` must not be NULL.
 * @pre     `arg_index` must be less than `MAX_IR_ARGS`.
 * @pre     The argument at `arg_index` must be of type `IR_TYPE_U1`.
 */
bool instruction_get_arg_u1(instruction_t *instruction, size_t arg_index);

/*!
 * @brief Retrieves an A32 register identifier argument from an instruction.
 *
 * @param instruction    Pointer to the IR instruction.
 * @param arg_index     The index of the argument to retrieve.
 *
 * @return  The `a32_register_t` identifier.
 * @pre     `instruction` must not be NULL.
 * @pre     `arg_index` must be less than `MAX_IR_ARGS`.
 * @pre     The argument at `arg_index` must be of type `IR_TYPE_A32_REGISTER`.
 */
pound::jit::a32_register_t instruction_get_arg_a32_register(
    instruction_t *instruction, size_t arg_index);

/*!
 * @brief Gets the return type of an instruction based on its opcode.
 *
 * @param instruction Pointer to the IR instruction.
 *
 * @return  The `type_t` that this instruction's opcode returns.
 * @pre     `instruction` must not be NULL.
 * @pre     `instruction->opcode` must be a valid opcode index (less than
 * `NUM_OPCODE`).
 */
type_t instruction_get_return_type(instruction_t *instruction);

/*!
 * @brief Gets the name of an instruction's opcode as a C-string.
 *
 * @param instruction Pointer to the IR instruction.
 *
 * @return  A constant C-string containing the opcode's name.
 * @pre     `instruction` must not be NULL.
 * @pre     `instruction->opcode` must be a valid opcode index (less than
 * `NUM_OPCODE`).
 * @pre     The global `g_opcodes` array must be initialized and accessible.
 */
const char *instruction_get_opcode_name(instruction_t *instruction);
}
#endif // POUND_JIT_IR_INSTRUCTION_H
