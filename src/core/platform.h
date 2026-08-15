#ifndef POUND_PLATFORM_H
#define POUND_PLATFORM_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if defined(_WIN32) || defined(_WIN64)

#define POUND_PLATFORM_WINDOWS 1
#define POUND_PLATFORM_APPLE   0
#define POUND_PLATFORM_LINUX   0
#define POUND_PLATFORM_POSIX   0

#elif defined(__APPLE__)

#define POUND_PLATFORM_WINDOWS 0
#define POUND_PLATFORM_APPLE   1
#define POUND_PLATFORM_LINUX   0
#define POUND_PLATFORM_POSIX   1

#elif defined(__linux__)

#define POUND_PLATFORM_WINDOWS 0
#define POUND_PLATFORM_APPLE   0
#define POUND_PLATFORM_LINUX   1
#define POUND_PLATFORM_POSIX   1

#else

#error "Unknown Platform"

#endif

#if defined(__aarch64__)

#define POUND_ARCHITECTURE_ARM 1
#define POUND_ARCHITECTURE_X86 0

#elif defined(__x86_64__) || defined(_M_X64)

#define POUND_ARCHITECTURE_ARM 0
#define POUND_ARCHITECTURE_X86 1

#else

#error "Ballistic requires a 64-bit ARM or x86 environment."

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* POUND_PLATFORM_H */

/*** end of file ***/
