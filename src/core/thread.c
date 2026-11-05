#define _GNU_SOURCE
#include "thread.h"
#include "attributes.h"
#include "log.h"
#include "memory/memory.h"
#include <stdio.h>
#include <string.h>

#if POUND_PLATFORM_WINDOWS

#include <process.h>

#else
#include <errno.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#endif // POUND_PLATFORM_WINDOWS

typedef struct
{
    thread_function_t function;
    void             *context;
    char              name[64];
    int               priority;
    char              pad[4];
} thread_entry_context_t;

#if POUND_PLATFORM_WINDOWS

static unsigned __stdcall
thread_windows_entry(void *p)
{
    thread_entry_context_t *entry_context = (thread_entry_context_t *)p;
    thread_function_t       function      = entry_context->function;
    void                   *context       = entry_context->context;

    const memory_bucket_type_t previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
    memory_subsystem_free(entry_context);
    (void)memory_subsystem_set_bucket(previous_bucket);

    function(context);
    return 0;
}

#else

static void *
thread_linux_entry(void *payload)
{
    thread_entry_context_t *POUND_RESTRICT entry_context = payload;
    thread_function_t                      function      = entry_context->function;
    void *POUND_RESTRICT                   context       = entry_context->context;
    int                                    priority      = entry_context->priority;

    const memory_bucket_type_t previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
    memory_subsystem_free(entry_context);
    (void)memory_subsystem_set_bucket(previous_bucket);

    if (priority != 0)
    {
        int nice_value;

        if (priority <= 0)
        {
            nice_value = priority * 20 / 15; // 0..-15 -> 0..-20
        }
        else
        {
            nice_value = priority * 19 / 15; // 1..15  -> 1..19
        }

        if (setpriority(PRIO_PROCESS, 0, nice_value) != 0 && errno != EPERM)
        {
            // Silently ignore EPERM (lack of root privileges to raise priority)
        }
    }

    function(context);
    return NULL;
}

#endif // POUND_PLATFORM_WINDOWS

pound_error_t
thread_create(thread_function_t           function,
              void *POUND_RESTRICT        context,
              size_t                      stack_size,
              thread_priority_t           priority,
              const char *POUND_RESTRICT  name,
              uint32_t                    affinity_mask,
              thread_id_t *POUND_RESTRICT out_id)
{
    const char *POUND_RESTRICT thread_name = "<unnamed>";

    if (POUND_UNLIKELY(NULL == out_id))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out_id is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    (void)memset(out_id, 0, sizeof(*out_id));

    if (POUND_UNLIKELY(NULL == function))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: function is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (name != NULL && name[0] != '\0')
    {
        thread_name = name;
    }

    if (0 == stack_size)
    {
        stack_size = THREAD_STACK_SIZE_DEFAULT;
    }

    if (POUND_UNLIKELY(stack_size < THREAD_STACK_SIZE_MINIMUM))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: thread '%s' requested stack size %zu,"
                        " which is below the minimum %zu.",
                        thread_name,
                        stack_size,
                        THREAD_STACK_SIZE_MINIMUM);
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(stack_size > THREAD_STACK_SIZE_MAXIMUM))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: thread '%s' requested stack size %zu,"
                        " which exceeds the maximum %zu.",
                        thread_name,
                        stack_size,
                        THREAD_STACK_SIZE_MAXIMUM);
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    const int priority_value = priority;
    if (POUND_UNLIKELY(priority_value < THREAD_PRIORITY_IDLE
                       || priority_value > THREAD_PRIORITY_TIME_CRITICAL))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: thread '%s' has invalid priority "
                        "%d. Expected range [%d, %d].",
                        thread_name,
                        priority_value,
                        THREAD_PRIORITY_IDLE,
                        THREAD_PRIORITY_TIME_CRITICAL);
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(THREAD_PRIORITY_TIME_CRITICAL == priority_value))
    {
        POUND_LOG_WARN(&thread_logger,
                       "Thread '%' is being created with TIME_CRITICAL "
                       "priority. This can starve other threads.",
                       thread_name);
    }

    if (affinity_mask != 0)
    {
        int processor_count = 0;

#if POUND_PLATFORM_WINDOWS

        SYSTEM_INFO system_info;
        (void)memset(&system_info, 0, sizeof(system_info));
        GetSystemInfo(&system_info);
        processor_count = (int)system_info.dwNumberOfProcessors;

#else

        processor_count = get_nprocs();

#endif // POUND_PLATFORM_WINDOWS

        if (POUND_UNLIKELY(processor_count <= 0))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: processor count %d is invalid"
                            " while validating affinity mask 0x%08X for thread '%s'.",
                            processor_count,
                            affinity_mask,
                            thread_name);
            return POUND_ERROR_INVALID_ARGUMENT;
        }

        uint32_t valid_mask = 0;

        if (processor_count >= 32)
        {
            valid_mask = UINT32_MAX;
        }
        else
        {
            valid_mask = (1U << processor_count) - 1U;
        }

        if (POUND_UNLIKELY((affinity_mask & ~valid_mask) != 0))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: affinity mask 0x%08X is "
                            "invalid for thread '%s'. System reported %d processors, valid "
                            "mask is 0x%08X.",
                            affinity_mask,
                            thread_name,
                            processor_count,
                            valid_mask);
            return POUND_ERROR_INVALID_ARGUMENT;
        }
    }

    memory_bucket_type_t previous_bucket  = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
    const size_t         memory_alignment = 16U;
    thread_entry_context_t *POUND_RESTRICT thread_entry_context
        = memory_subsystem_allocate(memory_alignment, sizeof(thread_entry_context_t));
    (void)memory_subsystem_set_bucket(previous_bucket);

    if (POUND_UNLIKELY(NULL == thread_entry_context))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to allocate %zu bytes for "
                        "thread '%s'.",
                        sizeof(thread_entry_context_t),
                        thread_name);
        return POUND_ERROR_ALLOCATION_FAILED;
    }

    (void)memset(thread_entry_context, 0, sizeof(*thread_entry_context));
    thread_entry_context->function = function;
    thread_entry_context->context  = context;

    if (NULL != name)
    {
        const int written
            = snprintf(thread_entry_context->name, sizeof(thread_entry_context->name), "%s", name);

        if (POUND_UNLIKELY(written < 0 || (size_t)written >= sizeof(thread_entry_context->name)))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: thread name '%s' is too long or invalid. "
                            "Maximum length is %zu bytes excluding null terminator.",
                            name,
                            sizeof(thread_entry_context->name) - 1);

            previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
            memory_subsystem_free(thread_entry_context);
            (void)memory_subsystem_set_bucket(previous_bucket);
            return POUND_ERROR_INVALID_ARGUMENT;
        }
    }
    else
    {
        thread_entry_context->name[0] = '\0';
    }

#if POUND_PLATFORM_WINDOWS

    if (POUND_UNLIKELY(stack_size > (size_t)UINT_MAX))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: thread '%s' stack size %zu is too large for Win32. "
                        "Maximum is %u.",
                        thread_name,
                        stack_size,
                        UINT_MAX);
        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    const uintptr_t raw_handle = _beginthreadex(NULL,
                                                (unsigned int)stack_size,
                                                thread_windows_entry,
                                                thread_entry_context,
                                                CREATE_SUSPENDED,
                                                NULL);

    if (POUND_UNLIKELY(0 == raw_handle))
    {
        const int   saved_errno = errno;
        const DWORD last_error  = GetLastError();

        char  error_text[256];
        DWORD message_length
            = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             last_error,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             error_text,
                             (DWORD)sizeof(error_text),
                             NULL);

        if (0 == message_length)
        {
            snprintf(error_text,
                     sizeof(error_text),
                     "Unknown Win32 error %lu",
                     (unsigned long)last_error);
        }

        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: _beginthreadex failed for thread '%s' because %s "
                        "(errno=%d, Win32=%lu).",
                        thread_name,
                        error_text,
                        saved_errno,
                        (unsigned long)last_error);

        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);

        return POUND_ERROR_THREAD_CREATION_FAILED;
    }

    HANDLE handle = raw_handle;

    if (POUND_UNLIKELY(0 == SetThreadPriority(handle, priority_value)))
    {
        const DWORD last_error = GetLastError();

        char  error_text[256];
        DWORD message_length
            = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             last_error,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             error_text,
                             (DWORD)sizeof(error_text),
                             NULL);

        if (0 == message_length)
        {
            snprintf(error_text,
                     sizeof(error_text),
                     "Unknown Win32 error %lu",
                     (unsigned long)last_error);
        }

        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SetThreadPriority failed for thread '%s' with priority "
                        "%d because %s "
                        "(Win32=%lu).",
                        thread_name,
                        priority_value,
                        error_text,
                        (unsigned long)last_error);

        CloseHandle(handle);
        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);

        return POUND_ERROR_THREAD_PRIORITY_FAILED;
    }

    if (0 == SetThreadPriorityBoost(handle, TRUE))
    {
        const DWORD last_error = GetLastError();

        POUND_LOG_WARN(
            &thread_logger,
            "SetThreadPriorityBoost failed for thread '%s' with Win32 error %lu. Continuing.",
            thread_name,
            (unsigned long)last_error);
    }

    if (affinity_mask != 0)
    {
        const DWORD_PTR previous_affinity = SetThreadAffinityMask(handle, (DWORD_PTR)affinity_mask);

        if (POUND_UNLIKELY(0 == previous_affinity))
        {
            const DWORD last_error = GetLastError();

            char  error_text[256];
            DWORD message_length
                = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 last_error,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 error_text,
                                 (DWORD)sizeof(error_text),
                                 NULL);

            if (0 == message_length)
            {
                snprintf(error_text,
                         sizeof(error_text),
                         "Unknown Win32 error %lu",
                         (unsigned long)last_error);
            }

            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: SetThreadAffinityMask failed for thread '%s' with "
                            "mask 0x%08X because %s "
                            "(Win32=%lu).",
                            thread_name,
                            affinity_mask,
                            error_text,
                            (unsigned long)last_error);

            CloseHandle(handle);
            previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
            memory_subsystem_free(thread_entry_context);
            (void)memory_subsystem_set_bucket(previous_bucket);

            return POUND_ERROR_THREAD_AFFINITY_FAILED;
        }
    }

    if ('\0' != start->name[0])
    {
        const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");

        if (NULL == kernel32)
        {
            POUND_LOG_WARN(&thread_logger,
                           "GetModuleHandleW(kernel32.dll) failed while naming "
                           "thread '%s', continuing...",
                           thread_name)
        }
        else
        {
            wchar_t   wide_name[64];
            const int wide_name_length
                = MultiByteToWideChar(CP_UTF8,
                                      0,
                                      start->name,
                                      -1,
                                      wide_name,
                                      (int)(sizeof(wide_name) / sizeof(wide_name[0])));

            if (wide_name_length <= 0)
            {
                POUND_LOG_WARN(&thread_logger,
                               "MultiByteToWideChar() failed while naming thread "
                               "'%s' with Win32 error %lu, continuing...",
                               threAD_name,
                               (unsigned long)GetLastError());
            }
            else
            {
                const HRESULT hr = set_description(handle, wide_name);

                if (hr < 0)
                {
                    POUND_LOG_WARN(&thread_logger,
                                   "SetThreadDescription failed for thread '%s' "
                                   "with HRESULT 0x%08lX, continuing...",
                                   thread_name,
                                   (unsigned long)hr);
                }
            }
        }
    }

    const DWORD previous_suspend_count = ResumeThread(handle);

    if (POUND_UNLIKELY((DWORD)-1 == previous_suspend_count))
    {
        const DWORD last_error = GetLastError();

        char  error_text[256];
        DWORD message_length
            = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             last_error,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             error_text,
                             (DWORD)sizeof(error_text),
                             NULL);

        if (0 == message_length)
        {
            snprintf(error_text,
                     sizeof(error_text),
                     "Unknown Win32 error %lu",
                     (unsigned long)last_error);
        }

        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: ResumeThread failed for thread '%s' because %s "
                        "(Win32=%lu).",
                        thread_name,
                        error_text,
                        (unsigned long)last_error);

        CloseHandle(handle);
        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);
        return POUND_ERROR_THREAD_CREATION_FAILED;
    }

    *out_id = handle;

    POUND_LOG_ERROR(&thread_logger,
                    "Create thread '%s' with stack_size=%zu, priority=%d, "
                    "affinity=0x%08X.",
                    thread_name,
                    stack_size,
                    priority_value,
                    affinity_mask);

    return POUND_SUCCESS;
#else

    pthread_attr_t attr;
    int            error = pthread_attr_init(&attr);

    if (POUND_UNLIKELY(error != 0))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: pthread_attr_init() failed with thread "
                        "'%s' because %s.",
                        thread_name,
                        strerror(error));

        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);
        return POUND_ERROR_THREAD_CREATION_FAILED;
    }

    error = pthread_attr_setstacksize(&attr, stack_size);

    if (POUND_UNLIKELY(error != 0))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: pthread_attr_setstacksize() failed "
                        "for thread '%s' with stack size %zu because %s.",
                        thread_name,
                        stack_size,
                        strerror(error));
        pthread_attr_destroy(&attr);
        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (POUND_UNLIKELY(error != 0))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: pthread_attr_setdetachstate() failed "
                        "for thread '%s' with because %s.",
                        thread_name,
                        strerror(error));
        pthread_attr_destroy(&attr);
        previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);
        return POUND_ERROR_THREAD_CREATION_FAILED;
    }

    if (affinity_mask != 0)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        for (int i = 0; i < 32; ++i)
        {
            if ((affinity_mask & (1U << i)) != 0)
            {
                CPU_SET(i, &cpuset);
            }
        }

        error = pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);

        if (POUND_UNLIKELY(error != 0))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: pthread_attr_setaffinity_np() "
                            "failed for thread '%s' with mask 0x%08X because %s.",
                            thread_name,
                            affinity_mask,
                            strerror(error));
            pthread_attr_destroy(&attr);
            previous_bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_SYSTEM);
            memory_subsystem_free(thread_entry_context);
            (void)memory_subsystem_set_bucket(previous_bucket);
            return POUND_ERROR_THREAD_AFFINITY_FAILED;
        }
    }

    thread_entry_context->priority = priority_value;
    thread_id_t handle;
    memset(&handle, 0, sizeof(handle));

    error = pthread_create(&handle, &attr, thread_linux_entry, thread_entry_context);
    const int destory_error = pthread_attr_destroy(&attr);

    if (POUND_UNLIKELY(destory_error != 0))
    {
        POUND_LOG_WARN(&thread_logger,
                       "pthread_attr_destory() failed for thread '%s' because %s,"
                       " continuing...",
                       thread_name,
                       strerror(destory_error));
    }

    if (POUND_UNLIKELY(error != 0))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "pthread_create() failed for thread '%s' because %s",
                        thread_name,
                        strerror(error));
        memory_subsystem_free(thread_entry_context);
        (void)memory_subsystem_set_bucket(previous_bucket);
        return POUND_ERROR_THREAD_CREATION_FAILED;
    }

    *out_id = handle;
    POUND_LOG_DEBUG(&thread_logger,
                    "Created thread '%s' with stack_size=%zu, priority=%d, "
                    "affinity=0x%08X.",
                    thread_name,
                    stack_size,
                    priority_value,
                    affinity_mask);

    return POUND_SUCCESS;

#endif // PLATFORM_PLATFORM_WINDOWS
}

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