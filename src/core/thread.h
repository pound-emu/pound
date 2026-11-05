#ifndef POUND_THREAD_H
#define POUND_THREAD_H

#include "errors.h"
#include "platform.h"
#include <stdbool.h>
#include <stdint.h>

#if POUND_PLATFORM_WINDOWS

#include <windows.h>
typedef HANDLE thread_id_t;

#else

#include <pthread.h>
typedef pthread_t thread_id_t;

#endif // POUND_PLATFORM_WINDOWS

#define THREAD_ID_INVALID         ((thread_id_t)0)
#define THREAD_STACK_SIZE_DEFAULT (1024U * 1024U)
#define THREAD_STACK_SIZE_MINIMUM (16U * 1024U)
#define THREAD_STACK_SIZE_MAXIMUM (1024U * 1024U * 1024U)

typedef void (*thread_function_t)(void *context);

typedef enum
{
    THREAD_PRIORITY_IDLE          = -15,
    THREAD_PRIORITY_LOWEST        = -2,
    THREAD_PRIORITY_BELOW_NORMAL  = -1,
    THREAD_PRIORITY_NORMAL        = 0,
    THREAD_PRIORITY_ABOVE_NORMAL  = 1,
    THREAD_PRIORITY_HIGHEST       = 2,
    THREAD_PRIORITY_TIME_CRITICAL = 15,
} thread_priority_t;

pound_error_t thread_create(thread_function_t function,
                            void             *context,
                            size_t            stack_size,
                            thread_priority_t priority,
                            const char       *name,
                            uint32_t          affinity_mask,
                            thread_id_t      *out_id);

/// Wait for thread to finish and release resources. Returns true on success.
bool thread_join(thread_id_t id);

/// Exit the current thread.
void thread_exit(void);

/// Yield execution to another thread.
void thread_yield(void);

/// Sleep for ms milliseconds.
void thread_sleep(unsigned ms);

/// Get current thread ID.
thread_id_t thread_get_current_id(void);

#endif // POUND_THREAD_H
