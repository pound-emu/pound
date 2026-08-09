#ifndef BALLISTIC_COMMON_ASSERT_H
#define BALLISTIC_COMMON_ASSERT_H

#include "bal_attributes.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    void bal_internal_assert_fail(const char *file,
                                  int         line,
                                  const char *func,
                                  const char *expr_str,
                                  const char *user_msg,
                                  ...);

#define BAL_ASSERT(expression)                                                               \
    do                                                                                       \
    {                                                                                        \
        if (!(expression))                                                                   \
        {                                                                                    \
            bal_internal_assert_fail(__FILE__, __LINE__, __func__, #expression, NULL, NULL); \
        }                                                                                    \
    } while (0)

#define BAL_ASSERT_MSG(expression, format, ...)                                                \
    do                                                                                         \
    {                                                                                          \
        if (!(expression))                                                                     \
        {                                                                                      \
            bal_internal_assert_fail(                                                          \
                __FILE__, __LINE__, __func__, #expression, format __VA_OPT__(, ) __VA_ARGS__); \
        }                                                                                      \
    } while (0)

#define BAL_UNREACHABLE(...)                              \
    bal_internal_assert_fail(__FILE__,                    \
                             __LINE__,                    \
                             __func__,                    \
                             "BAL_UNREACHABLE()",         \
                             "Unreachable code executed", \
                             ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BALLISTIC_COMMON_ASSERT_H
