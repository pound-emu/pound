#ifndef POUND_COMMON_MATH_H
#define POUND_COMMON_MATH_H
#include <stddef.h>
#include <stdbool.h>

/*!
 * @brief Performs a checked multiplication of two `size_t` values.
 *
 * @details
 * Calculates `a * b` and stores the result in the variable pointed to by `res`.
 *
 * @param[in] a
 *      The multiplicand.
 * @param[in] b
 *      The multiplier.
 * @param[out] res
 *      Pointer to the destination variable.
 *      **Precondition:** Must not be NULL.
 *      **Postcondition:** On success, contains `a * b`. On overflow, value is undefined.
 *
 * @retval false
 *      Success. The multiplication was performed safely.
 * @retval true
 *      **Overflow Detected.** The result exceeds `SIZE_MAX`.
 */
static inline bool safe_multiply_size_t(size_t a, size_t b, size_t* res)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_mul_overflow(a, b, res);
#else
    if (b > 0 && a > SIZE_MAX / b) return true;
    *res = a * b;
    return false;
#endif
}

/*!
 * @brief Performs a checked addition of two `size_t` values.
 *
 * @details
 * Calculates `a + b` and stores the result in the variable pointed to by `res`.
 *
 * @param[in] a
 *      The first addend.
 * @param[in] b
 *      The second addend.
 * @param[out] res
 *      Pointer to the destination variable.
 *      **Precondition:** Must not be NULL.
 *      **Postcondition:** On success, contains `a + b`. On overflow, value is undefined.
 *
 * @retval false
 *      Success. The addition was performed safely.
 * @retval true
 *      **Overflow Detected.** The result exceeds `SIZE_MAX`.
 */
static inline bool safe_add_size_t(size_t a, size_t b, size_t* res)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_add_overflow(a, b, res);
#else
    if (a > SIZE_MAX - b) return true;
    *res = a + b;
    return false;
#endif
}
#endif // POUND_COMMON_MATH_H
