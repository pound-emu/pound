#ifndef POUND_SAFE_MATH_H
#define POUND_SAFE_MATH_H

#include "attributes.h"
#include <stdint.h>

typedef enum
{
    POUND_MATH_SUCCESS = 0,
    POUND_MATH_ERROR_INVALID_ARGUMENT,
    POUND_MATH_ERROR_SIGNED_POSITIVE_OVERFLOW,
    POUND_MATH_ERROR_SIGNED_NEGATIVE_OVERFLOW,
    POUND_MATH_ERROR_UNSIGNED_OVERFLOW,
} safe_math_error_t;

POUND_CORE_API POUND_COLD const char *safe_math_error_string(safe_math_error_t error);

POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_u8(uint8_t a, uint8_t b, uint8_t *POUND_RESTRICT result);
POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_u16(uint16_t                 a,
                                              uint16_t                 b,
                                              uint16_t *POUND_RESTRICT result);
POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_u32(uint32_t                 a,
                                              uint32_t                 b,
                                              uint32_t *POUND_RESTRICT result);
POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_u64(uint64_t                 a,
                                              uint64_t                 b,
                                              uint64_t *POUND_RESTRICT result);

POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_i8(int8_t a, int8_t b, int8_t *POUND_RESTRICT result);
POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_i16(int16_t a, int16_t b, int16_t *POUND_RESTRICT result);
POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_i32(int32_t a, int32_t b, int32_t *POUND_RESTRICT result);
POUND_CORE_API POUND_HOT safe_math_error_t safe_math_add_i64(int64_t a, int64_t b, int64_t *POUND_RESTRICT result);

#endif // POUND_SAFE_MATH_H

/*** end of file ***/
