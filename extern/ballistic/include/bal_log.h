//! This module defines the logging system used by Ballistic. It routes log messages through a
//! user-provided callback so users can integrate this module into their application's logging
//! backend.
//!
//! # Configuration
//!
//! The verbosity is controlled by two mechanisms:
//!
//! 1. Compile Time: The `BAL_MAX_LOG_LEVEL` macro determine the maximum severity compiled into
//!    the binary. This is set in CMakeLists.txt. Logs below this level are compiled out via
//!    dead code elimination.
//! 2. Runtime: The `min_level` field in `bal_logger_t` filters messages dynamically.
//!
//! # Examples
//!
//! ## Default Initialization
//!
//! ```c
//! #include "bal_log.h"
//!
//! bal_logger_init_default();
//! BAL_LOG_INFO(&bal_thread_logger, "Engine initialized.");
//! ```
//!
//! ## Custom Backend
//!
//! ```c
//! #include "bal_log.h"
//! #include <stdio.h>
//!
//! void my_file_logger(void *user_data, bal_log_data_t *bal_data, const char *fmt, va_list args)
//! {
//!     FILE *f = (FILE *)user_data;
//!
//!     // Format: [LOG_LEVEL] file:line message
//!     fprintf(f, "[%d] %s:%d ", bal_data->level, bal_data->filename, bal_data->line);
//!     vfprintf(f, fmt, args);
//!     fprintf(f, "\n");
//! }
//!
//! // ---
//!
//! FILE *log_file = fopen("jit.log", "w");
//! bal_thread_logger.user_data = log_file;
//! bal_thread_logger.log = my_file_logger;
//! bal_thread_logger.min_level = BAL_LOG_LEVEL_DEBUG;
//! BAL_LOG_DEBUG(&bal_thread_logger, "Writing to custom file backend");
//! fclose(log_file);
//! ```

#ifndef BALLISTIC_LOGGING_H
#define BALLISTIC_LOGGING_H

#include "bal_attributes.h"
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if BAL_COMPILER_MSVC

#define BAL_FILENAME __FILE__

#else

#define BAL_FILENAME __FILE_NAME__

#endif // BAL_PLATFORM_MSVC

    /// Defines the severity of a log message.
    typedef enum
    {
        /// Logging is disabled.
        BAL_LOG_LEVEL_NONE = -1,

        /// Critical errors that likely result in immediate termination or undefined behaviour.
        BAL_LOG_LEVEL_ERROR = 0,

        /// Non-critical issues that may result in degraded performance or functionality loss.
        BAL_LOG_LEVEL_WARN = 1,

        /// General operational events.
        BAL_LOG_LEVEL_INFO = 2,

        /// Information useful for debugging logic errors.
        BAL_LOG_LEVEL_DEBUG = 3,

        /// Extreme verbose output.
        BAL_LOG_LEVEL_TRACE = 4,
    } bal_log_level_t;

    /// Metadata associated with a specific log event.
    typedef struct
    {
        /// Source file where the log occurred.
        const char *filename;

        /// The function name where the log occurred.
        const char *function;

        /// The log level.
        bal_log_level_t level;

        /// The line number where the log occurred
        int line;
    } bal_log_data_t;

    /// A function pointer defining a custom logging backend.
    ///
    /// Implementations of this function are responsible for formatting and persisting the  log
    /// message. The `user_data` pointer is passed through from the `bal_logger_t` context. The
    /// `bal_data` struct contains metadata about the event, while `format`, and `args` provide the
    /// standard printf-style message content.
    typedef void (*bal_log_function_t)(void           *user_data,
                                       bal_log_data_t *bal_data,
                                       const char     *format,
                                       va_list         args);

    /// The main logging context.
    typedef struct
    {
        /// An opaque pointer passed to the log callback. This can be used to store file handles or
        /// other context-specific data.
        void *user_data;

        /// The callback invoked when a message needs to be logged. If this is `NULL`, logging is
        /// disabled for this context.
        bal_log_function_t log;

        /// The minimum severity level required for a message to be processed.
        bal_log_level_t min_level;

        uint32_t pad;
    } bal_logger_t;

    extern BAL_THREAD_LOCAL bal_logger_t bal_thread_logger;

// Remove all log code if log level not defined.
//
#ifndef BAL_MAX_LOG_LEVEL

#define BAL_MAX_LOG_LEVEL BAL_LOG_LEVEL_ERROR

#endif

    /// Dispatches a log message to the configured backend.
    ///
    /// This is the entry point into the logging system. It invokes the callback defined in
    /// `logger`.
    ///
    /// # Warning
    ///
    /// Do not call this function directly. Use our logging macros like `BAL_LOG_INFO`.
    ///
    /// # Safety
    ///
    /// The `format` string must match the arguments provided in the variadic list, following
    /// standard `printf`.

    BAL_HOT void bal_log_message(const bal_logger_t *logger,
                                 bal_log_level_t     log_level,
                                 const char         *filename,
                                 const char         *function,
                                 int                 line,
                                 const char         *format,
                                 ...);

    /// Populates the thread local storage logger with Ballistic's default logging implementation.
    BAL_COLD void bal_logger_init_default(void);

#define BAL_LOG_ERROR(logger, format, ...)      \
    bal_log_message((logger),                   \
                    BAL_LOG_LEVEL_ERROR,        \
                    (const char *)BAL_FILENAME, \
                    (const char *)__FUNCTION__, \
                    __LINE__,                   \
                    format,                     \
                    ##__VA_ARGS__)
#define BAL_LOG_WARN(logger, format, ...) \
    bal_log_message(                      \
        (logger), BAL_LOG_LEVEL_WARN, BAL_FILENAME, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define BAL_LOG_INFO(logger, format, ...)       \
    bal_log_message((logger),                   \
                    BAL_LOG_LEVEL_INFO,         \
                    (const char *)BAL_FILENAME, \
                    (const char *)__FUNCTION__, \
                    __LINE__,                   \
                    format,                     \
                    ##__VA_ARGS__)

#ifndef NDEBUG
#define BAL_LOG_DEBUG(logger, format, ...) \
    bal_log_message((logger),              \
                    BAL_LOG_LEVEL_DEBUG,   \
                    BAL_FILENAME,          \
                    __FUNCTION__,          \
                    __LINE__,              \
                    format,                \
                    ##__VA_ARGS__)
#define BAL_LOG_TRACE(logger, format, ...) \
    bal_log_message((logger),              \
                    BAL_LOG_LEVEL_TRACE,   \
                    BAL_FILENAME,          \
                    __FUNCTION__,          \
                    __LINE__,              \
                    format,                \
                    ##__VA_ARGS__)
#else
// DEBUG and TRACE macros are compiled out completely in release builds.
#define BAL_LOG_DEBUG(logger, format, ...) \
    do                                     \
    {                                      \
    } while (0)
#define BAL_LOG_TRACE(logger, format, ...) \
    do                                     \
    {                                      \
    } while (0)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BALLISTIC_LOGGING_H */

/*** end of file ***/
