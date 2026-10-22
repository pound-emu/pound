//! Struct integrity checking for magic numbers.
//!
//! Every long-lived Ballistic struct carries a magic field. It is set to a known ALIVE pattern
//! on init() and a DEAD pattern on destroy(). All public functions verify the magic before doing
//! any work. This should make using Ballistic just a little bit safer. When a check fails, the
//! log output tells you exactly what went wrong.

#ifndef BALLISTIC_SAFETY_H
#define BALLISTIC_SAFETY_H

#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{



#endif // __cplusplus

#include <stdint.h>

#define BAL_MAGIC_UNINITIALIZED 0x00000000U

#define BAL_ASSEMBLER_MAGIC_ALIVE 0xBA11A550U // BALLISTO
#define BAL_ASSEMBLER_MAGIC_DEAD  0xDEADBA11U // DEADBALL
#define BAL_JIT_DEBUG_MAGIC_ALIVE 0x717DE801U // JITDEBUG
#define BAL_JIT_DEBUG_MAGIC_DEAD  0xDEAD717DU // DEADJIT

__attribute__((unused)) static const char *
bal_decode_magic(const uint32_t magic)
{
    switch (magic)
    {
        case BAL_MAGIC_UNINITIALIZED:
            return "Uninitialized Memory";
            break;
        case BAL_ASSEMBLER_MAGIC_ALIVE:
            return "BALLISTO";
            break;
        case BAL_ASSEMBLER_MAGIC_DEAD:
            return "DEADBALL";
            break;
        case BAL_JIT_DEBUG_MAGIC_ALIVE:
            return "JITDEBUG";
            break;
        case BAL_JIT_DEBUG_MAGIC_DEAD:
            return "DEADJIT";
            break;
        default:
            return "Unknown (Likely Buffer Overflow)";
            break;
    }
}

__attribute__((unused)) static const char *
bal_diagnose_magic_failure(const uint32_t actual_magic, const uint32_t dead_magic)
{
    if (actual_magic == BAL_MAGIC_UNINITIALIZED)
    {
        return "Struct was never initialized.";
    }
    if (actual_magic == dead_magic)
    {
        return "Struct was explicitly destroyed (Double Free?).";
    }
    return "Memory corruption or wrong struct type passed.";
}

__attribute__((unused)) static int
bal_pointer_is_aligned(const void *pointer, const size_t alignment)
{
    uintptr_t address;
    (void)memcpy(&address, &pointer, sizeof(address));
    const bool is_aligned = 0U == address % alignment;
    return is_aligned;
}

#define BAL_CHECK_MAGIC(ptr, alive_magic, dead_magic, struct_name_str)                           \
    do                                                                                           \
    {                                                                                            \
        if (BAL_UNLIKELY((ptr)->magic != (alive_magic)))                                         \
        {                                                                                        \
            BAL_LOG_ERROR(&bal_thread_logger,                                                    \
                          "\n================================================================\n" \
                          "%s INTEGRITY CHECK FAILED!\n"                                         \
                          "  Expected : 0x%08X (%s)\n"                                           \
                          "  Actual   : 0x%08X (%s)\n"                                           \
                          "  Reason: %s\n"                                                       \
                          "================================================================\n",  \
                          (struct_name_str),                                                     \
                          (alive_magic),                                                         \
                          bal_decode_magic(alive_magic),                                         \
                          (ptr)->magic,                                                          \
                          bal_decode_magic((ptr)->magic),                                        \
                          bal_diagnose_magic_failure((ptr)->magic, (dead_magic)));               \
            (ptr)->status = BAL_ERROR_STRUCT_CORRUPTED;                                          \
            return (BAL_ERROR_STRUCT_CORRUPTED);                                                 \
        }                                                                                        \
    } while (false)

#define BAL_CHECK_MAGIC_VOID(ptr, alive_magic, dead_magic, struct_name_str)                      \
    do                                                                                           \
    {                                                                                            \
        if (BAL_UNLIKELY((ptr)->magic != (alive_magic)))                                         \
        {                                                                                        \
            BAL_LOG_ERROR(&bal_thread_logger,                                                    \
                          "\n================================================================\n" \
                          "%s INTEGRITY CHECK FAILED!\n"                                         \
                          "  Expected : 0x%08X (%s)\n"                                           \
                          "  Actual   : 0x%08X (%s)\n"                                           \
                          "  Reason: %s\n"                                                       \
                          "================================================================\n",  \
                          (struct_name_str),                                                     \
                          (alive_magic),                                                         \
                          bal_decode_magic(alive_magic),                                         \
                          (ptr)->magic,                                                          \
                          bal_decode_magic((ptr)->magic),                                        \
                          bal_diagnose_magic_failure((ptr)->magic, (dead_magic)));               \
            (ptr)->status = BAL_ERROR_STRUCT_CORRUPTED;                                          \
            return;                                                                              \
        }                                                                                        \
    } while (false)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BALLISTIC_SAFETY_H