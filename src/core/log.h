#ifndef POUND_LOGGING_H
#define POUND_LOGGING_H

#include "attributes.h"
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if POUND_COMPILER_MSVC

#define POUND_FILENAME __FILE__

#else

#define POUND_FILENAME __FILE_NAME__

#endif // POUND_PLATFORM_MSVC

    /// Defines the severity of a log message.
    typedef enum
    {
        /// Logging is disabled.
        LOG_LEVEL_NONE = -1,

        /// Critical errors that likely result in immediate termination or undefined behaviour.
        LOG_LEVEL_ERROR = 0,

        /// Non-critical issues that may result in degraded performance or functionality loss.
        LOG_LEVEL_WARN = 1,

        /// General operational events.
        LOG_LEVEL_INFO = 2,

        /// Information useful for debugging logic errors.
        LOG_LEVEL_DEBUG = 3,

        /// Extreme verbose output.
        LOG_LEVEL_TRACE = 4,
    } log_level_t;

    /// Metadata associated with a specific log event.
    typedef struct
    {
        /// Source file where the log occurred.
        const char *filename;

        /// The function name where the log occurred.
        const char *function;

        /// The log level.
        log_level_t level;

        /// The line number where the log occurred
        int line;
    } log_data_t;

    /// A function pointer defining a custom logging backend.
    ///
    /// Implementations of this function are responsible for formatting and persisting the  log
    /// message. The `user_data` pointer is passed through from the `pound_logger_t` context. The
    /// `pound_data` struct contains metadata about the event, while `format`, and `args` provide
    /// the standard printf-style message content.
    typedef void (*log_function_t)(void       *user_data,
                                   log_data_t *pound_data,
                                   const char *format,
                                   va_list     args);

    /// The main logging context.
    typedef struct
    {
        /// An opaque pointer passed to the log callback. This can be used to store file handles or
        /// other context-specific data.
        void *user_data;

        /// The callback invoked when a message needs to be logged. If this is `NULL`, logging is
        /// disabled for this context.
        log_function_t log;

        /// The minimum severity level required for a message to be processed.
        log_level_t min_level;

        uint32_t pad;
    } logger_t;

    extern POUND_THREAD_LOCAL logger_t thread_logger;

    /// Dispatches a log message to the configured backend.
    ///
    /// This is the entry point into the logging system. It invokes the callback defined in
    /// `logger`.
    ///
    /// # Warning
    ///
    /// Do not call this function directly. Use our logging macros like `POUND_LOG_INFO`.
    ///
    /// # Safety
    ///
    /// The `format` string must match the arguments provided in the variadic list, following
    /// standard `printf`.

    POUND_HOT void pound_log_message(const logger_t *logger,
                                     log_level_t     log_level,
                                     const char     *filename,
                                     const char     *function,
                                     int             line,
                                     const char     *format,
                                     ...);

    /// Populates the thread local storage logger with Ballistic's default logging implementation.
    POUND_COLD void pound_logger_init_default(void);

#define POUND_LOG_ERROR(logger, format, ...)        \
    pound_log_message((logger),                     \
                      LOG_LEVEL_ERROR,              \
                      (const char *)POUND_FILENAME, \
                      (const char *)__FUNCTION__,   \
                      __LINE__,                     \
                      format,                       \
                      ##__VA_ARGS__)
#define POUND_LOG_WARN(logger, format, ...) \
    pound_log_message(                      \
        (logger), LOG_LEVEL_WARN, POUND_FILENAME, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define POUND_LOG_INFO(logger, format, ...)         \
    pound_log_message((logger),                     \
                      LOG_LEVEL_INFO,               \
                      (const char *)POUND_FILENAME, \
                      (const char *)__FUNCTION__,   \
                      __LINE__,                     \
                      format,                       \
                      ##__VA_ARGS__)

#ifndef NDEBUG
#define POUND_LOG_DEBUG(logger, format, ...) \
    pound_log_message(                       \
        (logger), LOG_LEVEL_DEBUG, POUND_FILENAME, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define POUND_LOG_TRACE(logger, format, ...) \
    pound_log_message(                       \
        (logger), LOG_LEVEL_TRACE, POUND_FILENAME, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
// DEBUG and TRACE macros are compiled out completely in release builds.
#define POUND_LOG_DEBUG(logger, format, ...) \
    do                                       \
    {                                        \
    } while (0)
#define POUND_LOG_TRACE(logger, format, ...) \
    do                                       \
    {                                        \
    } while (0)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* POUND_LOGGING_H */

/*** end of file ***/
