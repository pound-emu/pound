#include "thread.h"

#if POUND_PLATFORM_WINDOWS

#include <process.h>

#else

#include <sys/poll.h>
#include <unistd.h>

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

void
thread_sleep(unsigned ms)
{
#if POUND_PLATFORM_WINDOWS

    Sleep(ms);

#else

    while (ms > 0)
    {
        const unsigned chunk = ms > 1000000U ? 1000000U : ms;
        (void)poll(NULL, 0, (int)chunk);
        ms -= chunk;
    }

#endif // POUND_PLATFORM_WINDOWS
}

thread_id_t
thread_get_current_id(void)
{
#if POUND_PLATFORM_WINDOWS

    return GetCurrentThread();

#else

    return pthread_self();

#endif // POUND_PLATFORM_WINDOWS
}