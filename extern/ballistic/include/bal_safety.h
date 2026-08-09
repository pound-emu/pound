//! Struct integrity checking for magic numbers.
//!
//! Every long-lived Ballistic struct carries a magic field. It is set to a known ALIVE pattern
//! on init() and a DEAD pattern on destroy(). All public functions verify the magic before doing
//! any work. This should make using Ballistic just a little bit safer. When a check fails, the
//! log output tells you exactly what went wrong.

#ifndef BALLISTIC_SAFETY_H
#define BALLISTIC_SAFETY_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define BAL_MAGIC_UNINITIALIZED 0x00000000U

#define BAL_ASSEMBLER_MAGIC_ALIVE 0xBA11A550U // BALLISTO
#define BAL_ASSEMBLER_MAGIC_DEAD  0xDEADBA11U // DEADBALL
#define BAL_JIT_DEBUG_MAGIC_ALIVE 0x717DE801U // JITDEBUG
#define BAL_JIT_DEBUG_MAGIC_DEAD  0xDEAD717DU // DEADJIT

    static const char *bal_decode_magic(uint32_t magic)
    {
        switch (magic)
        {
            case BAL_MAGIC_UNINITIALIZED:
                return "Uninitialized Memory";
            case BAL_ASSEMBLER_MAGIC_ALIVE:
                return "BALLISTO";
            case BAL_ASSEMBLER_MAGIC_DEAD:
                return "DEADBALL";
            case BAL_JIT_DEBUG_MAGIC_ALIVE:
                return "JITDEBUG";
            case BAL_JIT_DEBUG_MAGIC_DEAD:
                return "DEADJIT";
            default:
                return "Unknown (Likely Buffer Overflow)";
        }
    }

    static inline const char *bal_diagnose_magic_failure(uint32_t actual_magic, uint32_t dead_magic)
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

#define BAL_CHECK_MAGIC(ptr, alive_magic, dead_magic, struct_name_str, logger)                   \
    do                                                                                           \
    {                                                                                            \
        if (BAL_UNLIKELY((ptr)->magic != (alive_magic)))                                         \
        {                                                                                        \
            BAL_LOG_ERROR(&logger,                                                               \
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
    } while (0)

#define BAL_CHECK_MAGIC_VOID(ptr, alive_magic, dead_magic, struct_name_str, logger)              \
    do                                                                                           \
    {                                                                                            \
        if (BAL_UNLIKELY((ptr)->magic != (alive_magic)))                                         \
        {                                                                                        \
            BAL_LOG_ERROR(&logger,                                                               \
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
    } while (0)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BALLISTIC_SAFETY_H
