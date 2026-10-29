#ifndef POUND_THREAD_H
#define POUND_THREAD_H

#include "platform.h"

#if POUND_PLATFORM_WINDOWS

#include <windows.h>
typedef HANDLE thread_id_t;

#else

#include <pthread.h>
typedef pthread_t thread_id_t;

#endif // POUND_PLATFORM_WINDOWS

#define THREAD_ID_INVALID ((thread_id_t)0)

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

/// Exit the current thread.
void thread_exit(void);

#endif // POUND_THREAD_H
