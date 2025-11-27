/**
 * @file value.h
 *
 * @brief Defines the `value_t` structure and associated manipulation functions
 * for the Pound JIT Intermediate Representation (IR).
 *
 * The `value_t` structure is designed to be a lightweight object
 * that can be efficiently created, copied, and passed around
 * during IR construction and optimization phases. It uses a tagged union
 * approach to store different types of data safely.
 */

#ifndef POUND_JIT_IR_VALUE_H
#define POUND_JIT_IR_VALUE_H
#include <stdint.h>
#include "type.h"
#include "jit/a32_types.h"

namespace pound::jit::ir {

/*!
 * @brief A polymorphic container for values in the JIT's Intermediate
 * Representation.
 *
 * The `value_t` struct represents a single value within the IR. It can hold
 * various types of data, identified by the `type` member. The actual data
 * is stored within the `inner` union.
 *
 * The `type` member must always correspond to the actual type of data
 * stored in the `inner` union. Using a union member that does not match
 * the `type` tag leads to undefined behavior.
 */
typedef struct
{
    type_t type;
    union
    {
        uint64_t                   immediate_u64;
        uint32_t                   immediate_u32;
        pound::jit::a32_register_t immediate_a32_register;
        uint8_t                    immediate_u8;
        bool                       immediate_u1;
    } inner;
} value_t;

/*!
 * @brief Initializes a `value_t` instance to a default/void state.
 *
 * @param value Pointer to the `value_t` instance to initialize.
 * @post The `value`'s `type` will be set to `IR_TYPE_VOID`.
 *       The contents of its `inner` union are considered undefined
 *       for a void value.
 */
void value_init(value_t *value);

/*!
 * @brief Initializes a `value_t` instance to hold an unsigned 64-bit immediate
 * value.
 *
 * @param value Pointer to the `value_t` instance to initialize.
 * @param u64 The 64-bit unsigned immediate value to store.
 * @post The `value`'s `type` will be set to `IR_TYPE_U64` and its
 *       `inner.immediate_u64` member will contain `u64`.
 */
void value_init_from_u64(value_t *value, uint64_t u64);

/*!
 * @brief Initializes a `value_t` instance to hold an unsigned 32-bit immediate
 * value.
 *
 * @param value Pointer to the `value_t` instance to initialize.
 * @param u32 The 32-bit unsigned immediate value to store.
 * @post The `value`'s `type` will be set to `IR_TYPE_U32` and its
 *       `inner.immediate_u32` member will contain `u32`.
 */
void value_init_from_u32(value_t *value, uint32_t u32);

/*!
 * @brief Initializes a `value_t` instance to hold an unsigned 8-bit immediate
 * value.
 *
 * @param value Pointer to the `value_t` instance to initialize.
 * @param u8 The 8-bit unsigned immediate value to store.
 * @post The `value`'s `type` will be set to `IR_TYPE_U8` and its
 *       `inner.immediate_u8` member will contain `u8`.
 */
void value_init_from_u8(value_t *value, uint8_t u8);

/*!
 * @brief Initializes a `value_t` instance to hold a 1-bit boolean immediate
 * value.
 *
 * @param value Pointer to the `value_t` instance to initialize.
 * @param u1 The boolean (1-bit) immediate value to store.
 * @post The `value`'s `type` will be set to `IR_TYPE_U1` and its
 *       `inner.immediate_u1` member will contain `u1`.
 */
void value_init_from_u1(value_t *value, bool u1);

/*!
 * @brief Initializes a `value_t` instance to hold an A32 register identifier.
 *
 * This function stores the *identity* of an A32 register (e.g., R0, SP, PC)
 * within the `value_t`. It does not store the *content* of that register.
 *
 * @param value Pointer to the `value_t` instance to initialize.
 * @param reg The A32 register identifier (of type `a32_register_t`) to store.
 * @post The `value`'s `type` will be set to `IR_TYPE_A32_REGISTER` and its
 *       `inner.immediate_a32_register` member will contain `reg`.
 */
void value_init_from_a32_register(value_t *value, a32_register_t reg);

/*!
 * @brief Retrieves an unsigned 64-bit immediate value from a `value_t`.
 *
 * @pre The `value` must be of type `IR_TYPE_U64`.
 * @param value Pointer to the `value_t` instance.
 * @retval uint64_t The 64-bit unsigned immediate value stored in `value`.
 * @warning Calling this function on a `value_t` not of type `IR_TYPE_U64`
 *          results in undefined behavior.
 */
uint64_t value_get_u64(const value_t *value);

/*!
 * @brief Retrieves an unsigned 32-bit immediate value from a `value_t`.
 *
 * @pre The `value` must be of type `IR_TYPE_U32`.
 * @param value Pointer to the `value_t` instance.
 * @retval uint32_t The 32-bit unsigned immediate value stored in `value`.
 * @warning Calling this function on a `value_t` not of type `IR_TYPE_U32`
 *          results in undefined behavior.
 */
uint32_t value_get_u32(const value_t *value);

/*!
 * @brief Retrieves an unsigned 8-bit immediate value from a `value_t`.
 *
 * @pre The `value` must be of type `IR_TYPE_U8`.
 * @param value Pointer to the `value_t` instance.
 * @retval uint8_t The 8-bit unsigned immediate value stored in `value`.
 * @warning Calling this function on a `value_t` not of type `IR_TYPE_U8`
 *          results in undefined behavior.
 */
uint8_t value_get_u8(const value_t *value);

/*!
 * @brief Retrieves an unsigned 1-bit immediate value from a `value_t`.
 *
 * @pre The `value` must be of type `IR_TYPE_U1`.
 * @param value Pointer to the `value_t` instance.
 * @retval bool The 1-bit unsigned immediate value stored in `value`.
 * @warning Calling this function on a `value_t` not of type `IR_TYPE_U1`
 *          results in undefined behavior.
 */
bool value_get_u1(const value_t *value);

/*!
 * @brief Retrieves an A32 register identifier from a `value_t`.
 *
 * @pre The `value` must be of type `IR_TYPE_A32_REGISTER`.
 * @param value Pointer to the `value_t` instance.
 * @retval pound::jit::a32_register_t The A32 register identifier stored in
 * `value`.
 * @warning Calling this function on a `value_t` not of type
 * `IR_TYPE_A32_REGISTER` results in undefined behavior.
 */
pound::jit::a32_register_t value_get_a32_register(const value_t *value);
} // namespace pound:::jit::ir
#endif // POUND_JIT_IR_TYPE_H
