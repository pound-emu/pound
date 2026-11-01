#define _GNU_SOURCE
#include "thread.h"

#if POUND_PLATFORM_WINDOWS

#include <process.h>

#else
#include <errno.h>
#include <sys/poll.h>
#include <sys/resource.h>
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

bool
thread_set_priority(thread_id_t id, thread_priority_t priority)
{
#if POUND_PLATFORM_WINDOWS

    return SetThreadPriority(id, priority) != 0;

#else

    int p = priority;

    if (p < -15)
    {
        p = -15;
    }

    if (p > 15)
    {
        p = 15;
    }

    // Linear remap: -15 -> -20, 0 -> 0, +15 -> +19
    int nice_value;

    if (p <= 0)
    {
        nice_value = p * 20 / 15; // 0..-15 -> 0..-20
    }
    else
    {
        nice_value = p * 19 / 15; // 1..15  -> 1..19
    }

    const pid_t tid = gettid();

    if (setpriority(PRIO_PROCESS, (id_t)tid, nice_value) != 0)
    {
        return errno == EPERM ? true : false;
    }

    return true;

#endif // POUND_PLATFORM_WINDOWS
}

thread_priority_t
thread_get_priority(thread_id_t id)
{
#if POUND_PLATFORM_WINDOWS

    return (thread_priority_t)GetThreadPriority(id);

#else

    pid_t tid;

    if (pthread_equal(id, pthread_self()))
    {
        tid = gettid();
    }
    else
    {
        return THREAD_PRIORITY_NORMAL;
    }

    errno                = 0;
    const int nice_value = getpriority(PRIO_PROCESS, (id_t)tid);

    if (errno != 0)
    {
        return THREAD_PRIORITY_NORMAL;
    }

    int priority;

    if (nice_value <= 0)
    {
        priority = nice_value * 15 / 20;
    }
    else
    {
        priority = nice_value * 15 / 19;
    }

    return (thread_priority_t)priority;

#endif // POUND_PLATFORM_WINDOWS
}