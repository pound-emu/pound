// Copyright 2025 Xenon Emulator Project. All rights reserved.

#include "Thread.h"
#include "Error.h"
#include "Logging/Log.h"

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <pthread.h>
#elif defined(_WIN32)
#include <Windows.h>
#include "StringUtil.h"
#else

#if defined(__Bitrig__) || defined(__DragonFly__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <pthread_np.h>
#else
#include <pthread.h>
#endif
#include <sched.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#include <algorithm>
#include <thread>

#ifdef __FreeBSD__
#define cpu_set_t cpuset_t
#endif

namespace Base
{

#ifdef __APPLE__

void SetCurrentThreadRealtime(const std::chrono::nanoseconds period_ns)
{
    // CPU time to grant.
    const std::chrono::nanoseconds computation_ns = period_ns / 2;

    // Determine the timebase for converting time to ticks.
    struct mach_timebase_info timebase{};
    mach_timebase_info(&timebase);
    const auto ticks_per_ns = static_cast<f64>(timebase.denom) / static_cast<f64>(timebase.numer);

    const auto period_ticks = static_cast<u32>(static_cast<f64>(period_ns.count()) * ticks_per_ns);
    const auto computation_ticks = static_cast<u32>(static_cast<f64>(computation_ns.count()) * ticks_per_ns);

    thread_time_constraint_policy policy = {
        .period = period_ticks,
        .computation = computation_ticks,
        // Should not matter since preemptible is false, but needs to be >= computation regardless.
        .constraint = computation_ticks,
        .preemptible = false,
    };

    int ret = thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY,
                                reinterpret_cast<thread_policy_t>(&policy), THREAD_TIME_CONSTRAINT_POLICY_COUNT);
    if (ret != KERN_SUCCESS)
    {
        LOG_ERROR(Base, "Could not set thread to real-time with period {} ns: {}", period_ns.count(), ret);
    }
}

#else

void SetCurrentThreadRealtime(const std::chrono::nanoseconds period_ns)
{
    // Not implemented
}

#endif

#ifdef _WIN32

void SetCurrentThreadPriority(ThreadPriority new_priority)
{
    const auto handle = GetCurrentThread();
    int windows_priority = 0;
    switch (new_priority)
    {
        case ThreadPriority::Low:
            windows_priority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case ThreadPriority::Normal:
            windows_priority = THREAD_PRIORITY_NORMAL;
            break;
        case ThreadPriority::High:
            windows_priority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case ThreadPriority::VeryHigh:
            windows_priority = THREAD_PRIORITY_HIGHEST;
            break;
        case ThreadPriority::Critical:
            windows_priority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        default:
            windows_priority = THREAD_PRIORITY_NORMAL;
            break;
    }
    SetThreadPriority(handle, windows_priority);
}

static void AccurateSleep(std::chrono::nanoseconds duration)
{
    LARGE_INTEGER interval{
        .QuadPart = -1 * (duration.count() / 100u),
    };
    const HANDLE timer = ::CreateWaitableTimer(nullptr, TRUE, nullptr);
    SetWaitableTimer(timer, &interval, 0, nullptr, nullptr, 0);
    WaitForSingleObject(timer, INFINITE);
    ::CloseHandle(timer);
}

#else

void SetCurrentThreadPriority(ThreadPriority new_priority)
{
    pthread_t this_thread = pthread_self();

    constexpr auto scheduling_type = SCHED_OTHER;
    const s32 max_prio = sched_get_priority_max(scheduling_type);
    const s32 min_prio = sched_get_priority_min(scheduling_type);
    const u32 level = std::max(static_cast<u32>(new_priority) + 1, 4U);

    struct sched_param params;
    if (max_prio > min_prio)
    {
        params.sched_priority = min_prio + ((max_prio - min_prio) * level) / 4;
    }
    else
    {
        params.sched_priority = min_prio - ((min_prio - max_prio) * level) / 4;
    }

    pthread_setschedparam(this_thread, scheduling_type, &params);
}

static void AccurateSleep(std::chrono::nanoseconds duration)
{
    std::this_thread::sleep_for(duration);
}

#endif

#if defined(_MSC_VER) || defined(__MINGW32__)

// Sets the debugger-visible name of the current thread.
void SetCurrentThreadName(const std::string_view& name)
{
    SetThreadDescription(GetCurrentThread(), UTF8ToUTF16W(name).data());
}

void SetThreadName(void* thread, const std::string_view& name)
{
    const char* nchar = name.data();
    std::string truncated(nchar, std::min(name.size(), static_cast<size_t>(15)));
    SetThreadDescription(thread, UTF8ToUTF16W(name).data());
}

#else  // !MSVC_VER, so must be POSIX threads

// MinGW with the POSIX threading model does not support pthread_setname_np
#if !defined(_WIN32) || defined(_MSC_VER)
void SetCurrentThreadName(const std::string_view& name)
{
    const char* nchar = name.data();
#ifdef __APPLE__
    pthread_setname_np(nchar);
#elif defined(__Bitrig__) || defined(__DragonFly__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    pthread_set_name_np(pthread_self(), nchar);
#elif defined(__NetBSD__)
    pthread_setname_np(pthread_self(), "%s", (void*)nchar);
#elif defined(__linux__)
    // Linux limits thread names to 15 characters and will outright reject any
    // attempt to set a longer name with ERANGE.
    std::string truncated(nchar, std::min(name.size(), static_cast<size_t>(15)));
    if (int e = pthread_setname_np(pthread_self(), truncated.c_str()))
    {
        errno = e;
    }
#else
    pthread_setname_np(pthread_self(), nchar);
#endif
}

void SetThreadName(void* thread, const std::string_view& name)
{
    // TODO
}
#endif

#if defined(_WIN32)
void SetCurrentThreadName(const std::string_view& name)
{
    // Do Nothing on MinGW
}

void SetThreadName(void* thread, const std::string_view& name)
{
    // Do Nothing on MinGW
}
#endif

#endif

AccurateTimer::AccurateTimer(std::chrono::nanoseconds target_interval) : target_interval(target_interval) {}

void AccurateTimer::Start()
{
    const auto begin_sleep = std::chrono::high_resolution_clock::now();
    if (total_wait.count() > 0)
    {
        AccurateSleep(total_wait);
    }
    start_time = std::chrono::high_resolution_clock::now();
    total_wait -= std::chrono::duration_cast<std::chrono::nanoseconds>(start_time - begin_sleep);
}

void AccurateTimer::End()
{
    const auto now = std::chrono::high_resolution_clock::now();
    total_wait += target_interval - std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time);
}

}  // namespace Base
