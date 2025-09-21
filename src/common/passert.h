#ifndef POUND_COMMON_ASSERT_H
#define POUND_COMMON_ASSERT_H

__attribute__((noreturn)) void pound_internal_assert_fail(const char* file, int line, const char* func,
                                                          const char* expr_str, const char* user_msg, ...);

#define PVM_ASSERT(expression)                                                                             \
    do                                                                                                     \
    {                                                                                                      \
        if (!(expression))                                                                                 \
        {                                                                                                  \
            pound_internal_assert_fail(__FILE__, __LINE__, __func__, #expression, nullptr, nullptr); \
        }                                                                                                  \
    } while (0)

#define PVM_ASSERT_MSG(expression, format, ...)                                                            \
    do                                                                                                     \
    {                                                                                                      \
        if (!(expression))                                                                                 \
        {                                                                                                  \
            pound_internal_assert_fail(__FILE__, __LINE__, __func__, #expression, format __VA_OPT__(,)  __VA_ARGS__); \
        }                                                                                                  \
    } while (0)

#define PVM_UNREACHABLE(...) pound_internal_assert_fail(__FILE__, __LINE__, __func__, "PVM_UNREACHABLE()", "Unreachable code executed", ##__VA_ARGS__);

#endif  // POUND_COMMON_ASSERT_H
