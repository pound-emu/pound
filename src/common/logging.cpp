#include "logging.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
/*
 * Stupid shit like this is why I hate Windows. WHY CANT YOU JUST FOLLOW
 * THE POSIX STAMDARD GOD DAMN.
 */
#define gmtime_r(src, dest) gmtime_s(dest, src)
#endif  // _WIN32

/*
 * TIMESTMP_BUFFER_LEN - The required buffer size for the timestamp format.
 *
 * Calculated for "%Y-%m-%dT%H:%M:%SZ":
 *   YYYY-mm-ddTHH:MM:SSZ
 *   4+1+2+1+2+1+2+1+2+1+2+1 = 20 characters
 *   +1 for the null terminator = 21 bytes.
 *
 * We define the buffer as 32 bytes to provide a safe margin for future
 * format changes, such as adding sub-second precision (e.g., ".123").
 */
#define TIMESTMP_BUFFER_LEN 32

/*
 * LOG_LINE_BUFFER_SIZE - A reasonable max size for a single log line.
 *
 * If it's too long, it will be truncated.
 */
#define LOG_LINE_BUFFER_SIZE 1024

/*
 * Static strings to use when all else fails.
 * Its unique format makes it easy to search for in logs.
 */
static const char* const FAILED_TIMESTAMP = "[TIMESTAMP_UNAVAILABLE]";
static const char* const FAILED_LOG_LEVEL = "[LOG_LEVEL_UNAVAILABLE]";

log_level_t runtime_log_level = LOG_LEVEL_NONE;

/*
 * Pre-allocate a buffer for the timestamp string.
 * Make it static so it's not constantly re-allocated on the stack.
 */
static char timestamp_buffer[TIMESTMP_BUFFER_LEN] = {};

const char* get_current_timestamp_str(void);

void log_message(log_level_t level, const char* module_name, const char* file, int line, const char* message, ...)
{
    assert(nullptr != message);

    const char* timestamp_str = get_current_timestamp_str();
    const char* level_str = nullptr;

    if (level < runtime_log_level)
    {
        return;
    }
    else if (level <= LOG_LEVEL_NONE)
    {
        level_str = FAILED_LOG_LEVEL;
    }
    else
    {
        switch (level)
        {
            case LOG_LEVEL_TRACE:
                level_str = "TRACE";
                break;
            case LOG_LEVEL_DEBUG:
                level_str = "DEBUG";
                break;
            case LOG_LEVEL_INFO:
                level_str = "INFO";
                break;
            case LOG_LEVEL_WARNING:
                level_str = "WARNING";
                break;
            case LOG_LEVEL_ERROR:
                level_str = "ERROR";
                break;
            case LOG_LEVEL_FATAL:
                level_str = "FATAL";
                break;
            default:
                level_str = FAILED_LOG_LEVEL;
                break;
        }
    }

    char buffer[LOG_LINE_BUFFER_SIZE] = {};

    /* Keep track of our position in the buffer */
    size_t offset = 0;
    offset += (size_t)snprintf(buffer + offset, LOG_LINE_BUFFER_SIZE - offset, "[%s] [%s] [%s] [%s:%d] ", timestamp_str,
                               level_str, module_name, file, line);

    /* Handle the user's variadic format string. */
    va_list args;
    va_start(args, message);

    if (offset < LOG_LINE_BUFFER_SIZE)
    {
        (void)vsnprintf(buffer + offset, LOG_LINE_BUFFER_SIZE - offset, message, args);
    }
    va_end(args);

    /* Print the entire, fully-formed buffer to stderr in a SINGLE, ATOMIC call. */
    fprintf(stderr, "%s\n", buffer);

    /*
     *  TODO(GloriousTaco:common): For guaranteed atomicity across
     *  threads, this entire function should be protected by a mutex. 
     *  The fprintf call itself is usually thread-safe at the *call* level, but
     *  our logic requires atomicity at the *message* level.
     *
     *  lock_logging_mutex();
     *  fprintf(stderr, "%s\n", buffer);
     *  unlock_logging_mutex();
     */
}

const char* get_current_timestamp_str(void)
{
    time_t now;

    /* time() can fail, though it's rare. */
    if (time(&now) == (time_t)-1)
    {
        return FAILED_TIMESTAMP;
    }

    struct tm time_since_epoch = {};
#ifdef WIN32
    /* https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/gmtime-s-gmtime32-s-gmtime64-s?view=msvc-170 */
    (void) gmtime_r(&now, &time_since_epoch);
    
#if 0
      TODO(GloriousTacoo:common): Someone fix this error handling. I have little
                                  patience for anything Windows related.
      if ((struct tm)-1 == &time_since_epoch)
      {
         return FAILED_TIMESTAMP;
      }
#endif

#else
    if (nullptr == gmtime_r(&now, &time_since_epoch))
    {
        return FAILED_TIMESTAMP;
    }
#endif  // WIN32

    size_t bytes_written = strftime(timestamp_buffer, TIMESTMP_BUFFER_LEN, "%Y-%m-%dT%H:%M:%SZ", &time_since_epoch);

    if (0 == bytes_written)
    {
        return FAILED_TIMESTAMP;
    }

    return timestamp_buffer;
}
