#ifndef POUND_COMMON_LOGGING_H
#define POUND_COMMON_LOGGING_H

/*
 * API USAGE CONTRACT:
 * -------------------
 *
 * Any source file (.c) that uses this logging framework MUST define
 * the LOG_MODULE macro *before* including this header file. This
 * provides a human-readable name for the component that is generating
 * the log message.
 *
 * Example Usage in a .c file:
 *
 * #define LOG_MODULE "KVM_MMU"
 * #include "common/logging.h"
 *
 * void mmu_translate_address(uint64_t gpa) {
 *      LOG_DEBUG("Translating GPA: 0x%llx", gpa);
 *      // ...
 *      if (error) {
 *          LOG_ERROR("Page table fault for GPA: 0x%llx", gpa);
 *      }
 * }
 */

/*
 * log_level_t - Defines the severity for log messages.
 */
typedef enum log_level
{
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
} log_level_t;

extern log_level_t runtime_log_level;

/*
 * This function is apart of the initial implementation and should NEVER be 
 * called directly. Use LOG_DEBUG, etc. macros instead.
 */
void log_message(log_level_t level, const char* module_name, const char* file, int line, const char* message, ...);

// -------------------------
// ---- Internal Macros ----
// -------------------------

#define __LOG_MODULE_NOT_DEFINED__
#ifndef LOG_MODULE
#define LOG_MODULE __LOG_MODULE_NOT_DEFINED__
#endif

/* DO NOT USE THESE DIRECTLY. They are internal helpers. */
#define _STRINGIFY_HELPER(x) #x
#define _STRINGIFY(x) _STRINGIFY_HELPER(x)

#define CHECK_LOG_MODULE_DEFINED()                                                                            \
    static_assert(__builtin_strcmp(_STRINGIFY(LOG_MODULE), _STRINGIFY(__LOG_MODULE_NOT_DEFINED__)) != 0,      \
                  "LOGGING ERROR: LOG_MODULE must be #defined before #including logging.h. Example: #define " \
                  "LOG_MODULE \"MY_MODULE\"")

#if COMPILE_TIME_LOG_LEVEL > LOG_LEVEL_TRACE
#define LOG_TRACE(format, ...) (void)0
#else
#define LOG_TRACE(format, ...)                                                               \
    do                                                                                       \
    {                                                                                        \
        CHECK_LOG_MODULE_DEFINED();                                                          \
        log_message(LOG_LEVEL_TRACE, LOG_MODULE, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    } while (0)
#endif

// -------------------------------
// ---- Public Logging Macros ----
// -------------------------------

#if COMPILE_TIME_LOG_LEVEL > LOG_LEVEL_DEBUG
#define LOG_DEBUG(format, ...) (void)0
#else
#define LOG_DEBUG(format, ...)                                                               \
    do                                                                                       \
    {                                                                                        \
        CHECK_LOG_MODULE_DEFINED();                                                          \
        log_message(LOG_LEVEL_DEBUG, LOG_MODULE, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    } while (0)
#endif

#if COMPILE_TIME_LOG_LEVEL > LOG_LEVEL_INFO
#define LOG_INFO(format, ...) (void)0
#else
#define LOG_INFO(format, ...)                                                               \
    do                                                                                      \
    {                                                                                       \
        CHECK_LOG_MODULE_DEFINED();                                                         \
        log_message(LOG_LEVEL_INFO, LOG_MODULE, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    } while (0)
#endif

#if COMPILE_TIME_LOG_LEVEL > LOG_LEVEL_WARNING
#define LOG_WARNING(format, ...) (void)0
#else
#define LOG_WARNING(format, ...)                                                               \
    do                                                                                         \
    {                                                                                          \
        CHECK_LOG_MODULE_DEFINED();                                                            \
        log_message(LOG_LEVEL_WARNING, LOG_MODULE, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    } while (0)
#endif

#if COMPILE_TIME_LOG_LEVEL > LOG_LEVEL_ERROR
#define LOG_ERROR(format, ...) (void)0
#else
#define LOG_ERROR(format, ...)                                                               \
    do                                                                                       \
    {                                                                                        \
        CHECK_LOG_MODULE_DEFINED();                                                          \
        log_message(LOG_LEVEL_ERROR, LOG_MODULE, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    } while (0)
#endif

#if COMPILE_TIME_LOG_LEVEL > LOG_LEVEL_FATAL
#define LOG_FATAL(format, ...) (void)0
#else
#define LOG_FATAL(format, ...)                                                               \
    do                                                                                       \
    {                                                                                        \
        CHECK_LOG_MODULE_DEFINED();                                                          \
        log_message(LOG_LEVEL_FATAL, LOG_MODULE, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    } while (0)
#endif
#endif  // POUND_COMMON_LOGGING_H
