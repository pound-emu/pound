#ifndef BALLISTIC_PLATFORM_H
#define BALLISTIC_PLATFORM_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if defined(_WIN32) || defined(_WIN64)

#define BAL_PLATFORM_WINDOWS 1
#define BAL_PLATFORM_APPLE   0
#define BAL_PLATFORM_LINUX   0
#define BAL_PLATFORM_POSIX   0

#elif defined(__APPLE__)

#define BAL_PLATFORM_WINDOWS 0
#define BAL_PLATFORM_APPLE   1
#define BAL_PLATFORM_LINUX   0
#define BAL_PLATFORM_POSIX   1

#elif defined(__linux__)

#define BAL_PLATFORM_WINDOWS 0
#define BAL_PLATFORM_APPLE   0
#define BAL_PLATFORM_LINUX   1
#define BAL_PLATFORM_POSIX   1

#else

#error "Unknown Platform"

#endif

#if defined(__aarch64__)

#define BAL_ARCHITECTURE_ARM 1
#define BAL_ARCHITECTURE_X86 0

#elif defined(__x86_64__) || defined(_M_X64)

#define BAL_ARCHITECTURE_ARM 0
#define BAL_ARCHITECTURE_X86 1

#else

#error "Ballistic requires a 64-bit ARM or x86 environment."

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BALLISTIC_PLATFORM_H */

/*** end of file ***/
