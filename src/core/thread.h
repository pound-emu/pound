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

#endif // POUND_THREAD_H
