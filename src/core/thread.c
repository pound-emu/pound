#include "thread.h"

#if POUND_PLATFORM_WINDOWS

#include <process.h>

#endif // POUND_PLATFORM_WINDOWS

void
thread_exit(void)
{
#if POUND_PLATFORM_WINDOWS

    _endthreadex(0);

#else

    pthread_exit(NULL);

#endif // POUND_PLATFORM_WINDOWS
}