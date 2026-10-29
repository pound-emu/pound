#include "thread.h"

#if POUND_PLATFORM_WINDOWS

#include <process.h>

#endif // POUND_PLATFORM_WINDOWS

bool
thread_join(thread_id_t id)
{
    if (THREAD_ID_INVALID == id)
    {
        return false;
    }

#if POUND_PLATFORM_WINDOWS

    DWORD r = WaitForSingleObject(id, INFINITE);
    CloseHandle(id);
    return WAIT_OBJECT_0 == r;

#else

    return pthread_join(id, NULL) == 0;

#endif // POUND_PLATFORM_WINDOWS
}

void
thread_exit(void)
{
#if POUND_PLATFORM_WINDOWS

    _endthreadex(0);

#else

    pthread_exit(NULL);

#endif // POUND_PLATFORM_WINDOWS
}

void
thread_yield(void)
{
#if POUND_PLATFORM_WINDOWS

    SwitchToThread();

#else

    sched_yield();

#endif // POUND_PLATFORM_WINDOWS
}