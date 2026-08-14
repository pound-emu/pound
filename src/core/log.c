#include "log.h"
#include "attributes.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

POUND_THREAD_LOCAL logger_t thread_logger = { 0 };

POUND_WEAK POUND_HOT void pound_default_logger(void       *user_data,
                                               log_data_t *pound_data,
                                               const char *format,
                                               va_list     args);

POUND_WEAK void
pound_log_message(const logger_t   *logger,
                  const log_level_t log_level,
                  const char       *filename,
                  const char       *function,
                  const int         line,
                  const char       *format,
                  ...)
{
    if ((uintptr_t)logger == (uintptr_t)NULL)
    {
        return;
    }

    if (logger->log && log_level <= logger->min_level)
    {
        log_data_t log_data
            = { .filename = filename, .function = function, .level = log_level, .line = line };

        va_list args;
        va_start(args, format);
        logger->log(logger->user_data, &log_data, format, args);
        va_end(args);
    }
}

POUND_WEAK void
pound_logger_init_default(void)
{
    thread_logger.log       = pound_default_logger;
    thread_logger.min_level = LOG_LEVEL_TRACE;
}

void
pound_default_logger(void *user_data, log_data_t *data, const char *format, va_list args)
{
    (void)user_data;
    const char *level_string = "MISSING";

    switch (data->level)
    {
        case LOG_LEVEL_NONE: {
            level_string = "NONE";
            break;
        }
        case LOG_LEVEL_ERROR: {
            level_string = "ERROR";
            break;
        }

        case LOG_LEVEL_WARN: {
            level_string = "WARN";
            break;
        }

        case LOG_LEVEL_INFO: {
            level_string = "INFO";
            break;
        }

        case LOG_LEVEL_DEBUG: {
            level_string = "DEBUG";
            break;
        }

        case LOG_LEVEL_TRACE: {
            level_string = "TRACE";
            break;
        }
    }

    const char *POUND_RESTRICT filename   = data->filename;
    const char *POUND_RESTRICT slash      = strrchr(filename, '/');
    const char *POUND_RESTRICT backslash  = strrchr(filename, '\\');
    const char *POUND_RESTRICT last_slash = slash > backslash ? slash : backslash;

    if (last_slash)
    {
        filename = last_slash;
    }

    fprintf(stderr, "[%s] [%s] [%s:%d] ", level_string, data->function, filename, data->line);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

/*** end of file ***/
