/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_build_config.h
 *
 *  This is a set of defines to configure the SDL features
 */

#ifndef SDL_build_config_h_
#define SDL_build_config_h_

/* General platform specific identifiers */
#include <SDL3/SDL_platform_defines.h>

/* #undef HAVE_GCC_ATOMICS */
/* #undef HAVE_GCC_SYNC_LOCK_TEST_AND_SET */

#define SDL_DISABLE_ALLOCA 1

/* Useful headers */
#define HAVE_FLOAT_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1

/* Comment this if you want to build without any C library requirements */
#define HAVE_LIBC 1
#ifdef HAVE_LIBC

/* Useful headers */
/* #undef HAVE_ALLOCA_H */
/* #undef HAVE_ICONV_H */
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MATH_H 1
#define HAVE_MEMORY_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
/* #undef HAVE_STRINGS_H */
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_WCHAR_H 1
/* #undef HAVE_PTHREAD_NP_H */

/* C library functions */
/* #undef HAVE_DLOPEN */
#define HAVE_MALLOC 1
/* #undef HAVE_FDATASYNC */
#define HAVE_GETENV 1
/* #undef HAVE_GETHOSTNAME */
/* #undef HAVE_SETENV */
#define HAVE_PUTENV 1
/* #undef HAVE_UNSETENV */
#define HAVE_ABS 1
/* #undef HAVE_BCOPY */
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_WCSLEN 1
#define HAVE_WCSNLEN 1
/* #undef HAVE_WCSLCPY */
/* #undef HAVE_WCSLCAT */
#define HAVE_WCSSTR 1
#define HAVE_WCSCMP 1
#define HAVE_WCSNCMP 1
#define HAVE_WCSTOL 1
#define HAVE_STRLEN 1
#define HAVE_STRNLEN 1
/* #undef HAVE_STRLCPY */
/* #undef HAVE_STRLCAT */
#define HAVE_STRPBRK 1
#define HAVE__STRREV 1
/* #undef HAVE_INDEX */
/* #undef HAVE_RINDEX */
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
/* #undef HAVE_STRNSTR */
/* #undef HAVE_STRTOK_R */
#define HAVE_ITOA 1
#define HAVE__LTOA 1
/* #undef HAVE__UITOA */
#define HAVE__ULTOA 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE__I64TOA 1
#define HAVE__UI64TOA 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE_VSSCANF 1
#define HAVE_VSNPRINTF 1
#define HAVE_ACOS 1
#define HAVE_ACOSF 1
#define HAVE_ASIN 1
#define HAVE_ASINF 1
#define HAVE_ATAN 1
#define HAVE_ATANF 1
#define HAVE_ATAN2 1
#define HAVE_ATAN2F 1
#define HAVE_CEIL 1
#define HAVE_CEILF 1
#define HAVE_COPYSIGN 1
#define HAVE_COPYSIGNF 1
#define HAVE__COPYSIGN 1
#define HAVE_COS 1
#define HAVE_COSF 1
#define HAVE_EXP 1
#define HAVE_EXPF 1
#define HAVE_FABS 1
#define HAVE_FABSF 1
#define HAVE_FLOOR 1
#define HAVE_FLOORF 1
#define HAVE_FMOD 1
#define HAVE_FMODF 1
#define HAVE_ISINF 1
/* #undef HAVE_ISINFF */
#define HAVE_ISINF_FLOAT_MACRO 1
#define HAVE_ISNAN 1
/* #undef HAVE_ISNANF */
#define HAVE_ISNAN_FLOAT_MACRO 1
#define HAVE_LOG 1
#define HAVE_LOGF 1
#define HAVE_LOG10 1
#define HAVE_LOG10F 1
#define HAVE_LROUND 1
#define HAVE_LROUNDF 1
#define HAVE_MODF 1
#define HAVE_MODFF 1
#define HAVE_POW 1
#define HAVE_POWF 1
#define HAVE_ROUND 1
#define HAVE_ROUNDF 1
#define HAVE_SCALBN 1
#define HAVE_SCALBNF 1
#define HAVE_SIN 1
#define HAVE_SINF 1
#define HAVE_SQRT 1
#define HAVE_SQRTF 1
#define HAVE_TAN 1
#define HAVE_TANF 1
#define HAVE_TRUNC 1
#define HAVE_TRUNCF 1
#define HAVE__FSEEKI64 1
/* #undef HAVE_FOPEN64 */
/* #undef HAVE_FSEEKO */
/* #undef HAVE_FSEEKO64 */
/* #undef HAVE_MEMFD_CREATE */
/* #undef HAVE_POSIX_FALLOCATE */
/* #undef HAVE_SIGACTION */
/* #undef HAVE_SA_SIGACTION */
/* #undef HAVE_ST_MTIM */
/* #undef HAVE_SETJMP */
/* #undef HAVE_NANOSLEEP */
/* #undef HAVE_GMTIME_R */
/* #undef HAVE_LOCALTIME_R */
/* #undef HAVE_NL_LANGINFO */
/* #undef HAVE_SYSCONF */
/* #undef HAVE_SYSCTLBYNAME */
/* #undef HAVE_CLOCK_GETTIME */
/* #undef HAVE_GETPAGESIZE */
/* #undef HAVE_ICONV */
/* #undef SDL_USE_LIBICONV */
/* #undef HAVE_PTHREAD_SETNAME_NP */
/* #undef HAVE_PTHREAD_SET_NAME_NP */
/* #undef HAVE_SEM_TIMEDWAIT */
/* #undef HAVE_GETAUXVAL */
/* #undef HAVE_ELF_AUX_INFO */
/* #undef HAVE_POLL */
#define HAVE__EXIT 1

#endif /* HAVE_LIBC */

/* #undef HAVE_DBUS_DBUS_H */
/* #undef HAVE_FCITX */
/* #undef HAVE_IBUS_IBUS_H */
/* #undef HAVE_INOTIFY_INIT1 */
/* #undef HAVE_INOTIFY */
/* #undef HAVE_LIBUSB */
/* #undef HAVE_O_CLOEXEC */

/* #undef HAVE_LINUX_INPUT_H */
/* #undef HAVE_LIBUDEV_H */
/* #undef HAVE_LIBDECOR_H */
/* #undef HAVE_LIBURING_H */

#define HAVE_DDRAW_H 1
#define HAVE_DSOUND_H 1
#define HAVE_DINPUT_H 1
#define HAVE_XINPUT_H 1
#define HAVE_WINDOWS_GAMING_INPUT_H 1
#define HAVE_GAMEINPUT_H 1
#define HAVE_DXGI_H 1
#define HAVE_DXGI1_6_H 1

#define HAVE_MMDEVICEAPI_H 1
#define HAVE_TPCSHRD_H 1
#define HAVE_ROAPI_H 1
#define HAVE_SHELLSCALINGAPI_H 1

/* #undef USE_POSIX_SPAWN */

/* SDL internal assertion support */
/* #undef SDL_DEFAULT_ASSERT_LEVEL_CONFIGURED */
#ifdef SDL_DEFAULT_ASSERT_LEVEL_CONFIGURED
#define SDL_DEFAULT_ASSERT_LEVEL 
#endif

/* Allow disabling of major subsystems */
/* #undef SDL_AUDIO_DISABLED */
/* #undef SDL_VIDEO_DISABLED */
/* #undef SDL_GPU_DISABLED */
/* #undef SDL_RENDER_DISABLED */
/* #undef SDL_CAMERA_DISABLED */
/* #undef SDL_JOYSTICK_DISABLED */
/* #undef SDL_HAPTIC_DISABLED */
/* #undef SDL_HIDAPI_DISABLED */
/* #undef SDL_POWER_DISABLED */
/* #undef SDL_SENSOR_DISABLED */
/* #undef SDL_DIALOG_DISABLED */
/* #undef SDL_THREADS_DISABLED */

/* Enable various audio drivers */
/* #undef SDL_AUDIO_DRIVER_ALSA */
/* #undef SDL_AUDIO_DRIVER_ALSA_DYNAMIC */
/* #undef SDL_AUDIO_DRIVER_OPENSLES */
/* #undef SDL_AUDIO_DRIVER_AAUDIO */
/* #undef SDL_AUDIO_DRIVER_COREAUDIO */
/* #undef SDL_AUDIO_DRIVER_DISK */
#define SDL_AUDIO_DRIVER_DSOUND 1
#define SDL_AUDIO_DRIVER_DUMMY 1
/* #undef SDL_AUDIO_DRIVER_EMSCRIPTEN */
/* #undef SDL_AUDIO_DRIVER_HAIKU */
/* #undef SDL_AUDIO_DRIVER_JACK */
/* #undef SDL_AUDIO_DRIVER_JACK_DYNAMIC */
/* #undef SDL_AUDIO_DRIVER_NETBSD */
/* #undef SDL_AUDIO_DRIVER_OSS */
/* #undef SDL_AUDIO_DRIVER_PIPEWIRE */
/* #undef SDL_AUDIO_DRIVER_PIPEWIRE_DYNAMIC */
/* #undef SDL_AUDIO_DRIVER_PULSEAUDIO */
/* #undef SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC */
/* #undef SDL_AUDIO_DRIVER_SNDIO */
/* #undef SDL_AUDIO_DRIVER_SNDIO_DYNAMIC */
#define SDL_AUDIO_DRIVER_WASAPI 1
/* #undef SDL_AUDIO_DRIVER_VITA */
/* #undef SDL_AUDIO_DRIVER_PSP */
/* #undef SDL_AUDIO_DRIVER_PS2 */
/* #undef SDL_AUDIO_DRIVER_N3DS */
/* #undef SDL_AUDIO_DRIVER_QNX */

/* Enable various input drivers */
/* #undef SDL_INPUT_LINUXEV */
/* #undef SDL_INPUT_LINUXKD */
/* #undef SDL_INPUT_FBSDKBIO */
/* #undef SDL_INPUT_WSCONS */
/* #undef SDL_HAVE_MACHINE_JOYSTICK_H */
/* #undef SDL_JOYSTICK_ANDROID */
#define SDL_JOYSTICK_DINPUT 1
/* #undef SDL_JOYSTICK_DUMMY */
/* #undef SDL_JOYSTICK_EMSCRIPTEN */
#define SDL_JOYSTICK_GAMEINPUT 1
/* #undef SDL_JOYSTICK_HAIKU */
#define SDL_JOYSTICK_HIDAPI 1
/* #undef SDL_JOYSTICK_IOKIT */
/* #undef SDL_JOYSTICK_LINUX */
/* #undef SDL_JOYSTICK_MFI */
/* #undef SDL_JOYSTICK_N3DS */
/* #undef SDL_JOYSTICK_PS2 */
/* #undef SDL_JOYSTICK_PSP */
#define SDL_JOYSTICK_RAWINPUT 1
/* #undef SDL_JOYSTICK_USBHID */
#define SDL_JOYSTICK_VIRTUAL 1
/* #undef SDL_JOYSTICK_VITA */
#define SDL_JOYSTICK_WGI 1
#define SDL_JOYSTICK_XINPUT 1
/* #undef SDL_HAPTIC_DUMMY */
/* #undef SDL_HAPTIC_LINUX */
/* #undef SDL_HAPTIC_IOKIT */
#define SDL_HAPTIC_DINPUT 1
/* #undef SDL_HAPTIC_ANDROID */
/* #undef SDL_LIBUSB_DYNAMIC */
/* #undef SDL_UDEV_DYNAMIC */

/* Enable various process implementations */
/* #undef SDL_PROCESS_DUMMY */
/* #undef SDL_PROCESS_POSIX */
#define SDL_PROCESS_WINDOWS 1

/* Enable various sensor drivers */
/* #undef SDL_SENSOR_ANDROID */
/* #undef SDL_SENSOR_COREMOTION */
#define SDL_SENSOR_WINDOWS 1
/* #undef SDL_SENSOR_DUMMY */
/* #undef SDL_SENSOR_VITA */
/* #undef SDL_SENSOR_N3DS */

/* Enable various shared object loading systems */
/* #undef SDL_LOADSO_DLOPEN */
/* #undef SDL_LOADSO_DUMMY */
#define SDL_LOADSO_WINDOWS 1

/* Enable various threading systems */
#define SDL_THREAD_GENERIC_COND_SUFFIX 1
#define SDL_THREAD_GENERIC_RWLOCK_SUFFIX 1
/* #undef SDL_THREAD_PTHREAD */
/* #undef SDL_THREAD_PTHREAD_RECURSIVE_MUTEX */
/* #undef SDL_THREAD_PTHREAD_RECURSIVE_MUTEX_NP */
#define SDL_THREAD_WINDOWS 1
/* #undef SDL_THREAD_VITA */
/* #undef SDL_THREAD_PSP */
/* #undef SDL_THREAD_PS2 */
/* #undef SDL_THREAD_N3DS */

/* Enable various RTC systems */
/* #undef SDL_TIME_UNIX */
#define SDL_TIME_WINDOWS 1
/* #undef SDL_TIME_VITA */
/* #undef SDL_TIME_PSP */
/* #undef SDL_TIME_PS2 */
/* #undef SDL_TIME_N3DS */

/* Enable various timer systems */
/* #undef SDL_TIMER_HAIKU */
/* #undef SDL_TIMER_UNIX */
#define SDL_TIMER_WINDOWS 1
/* #undef SDL_TIMER_VITA */
/* #undef SDL_TIMER_PSP */
/* #undef SDL_TIMER_PS2 */
/* #undef SDL_TIMER_N3DS */

/* Enable various video drivers */
/* #undef SDL_VIDEO_DRIVER_ANDROID */
/* #undef SDL_VIDEO_DRIVER_COCOA */
#define SDL_VIDEO_DRIVER_DUMMY 1
/* #undef SDL_VIDEO_DRIVER_EMSCRIPTEN */
/* #undef SDL_VIDEO_DRIVER_HAIKU */
/* #undef SDL_VIDEO_DRIVER_KMSDRM */
/* #undef SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC */
/* #undef SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC_GBM */
/* #undef SDL_VIDEO_DRIVER_N3DS */
#define SDL_VIDEO_DRIVER_OFFSCREEN 1
/* #undef SDL_VIDEO_DRIVER_PS2 */
/* #undef SDL_VIDEO_DRIVER_PSP */
/* #undef SDL_VIDEO_DRIVER_RISCOS */
/* #undef SDL_VIDEO_DRIVER_ROCKCHIP */
/* #undef SDL_VIDEO_DRIVER_RPI */
/* #undef SDL_VIDEO_DRIVER_UIKIT */
/* #undef SDL_VIDEO_DRIVER_VITA */
/* #undef SDL_VIDEO_DRIVER_VIVANTE */
/* #undef SDL_VIDEO_DRIVER_VIVANTE_VDK */
/* #undef SDL_VIDEO_DRIVER_OPENVR */
/* #undef SDL_VIDEO_DRIVER_WAYLAND */
/* #undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC */
/* #undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR */
/* #undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL */
/* #undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR */
/* #undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON */
#define SDL_VIDEO_DRIVER_WINDOWS 1
/* #undef SDL_VIDEO_DRIVER_X11 */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XFIXES */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT2 */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR */
/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS */
/* #undef SDL_VIDEO_DRIVER_X11_HAS_XKBLOOKUPKEYSYM */
/* #undef SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS */
/* #undef SDL_VIDEO_DRIVER_X11_XCURSOR */
/* #undef SDL_VIDEO_DRIVER_X11_XDBE */
/* #undef SDL_VIDEO_DRIVER_X11_XFIXES */
/* #undef SDL_VIDEO_DRIVER_X11_XINPUT2 */
/* #undef SDL_VIDEO_DRIVER_X11_XINPUT2_SUPPORTS_MULTITOUCH */
/* #undef SDL_VIDEO_DRIVER_X11_XRANDR */
/* #undef SDL_VIDEO_DRIVER_X11_XSCRNSAVER */
/* #undef SDL_VIDEO_DRIVER_X11_XSHAPE */
/* #undef SDL_VIDEO_DRIVER_X11_XSYNC */
/* #undef SDL_VIDEO_DRIVER_QNX */

#define SDL_VIDEO_RENDER_D3D 1
#define SDL_VIDEO_RENDER_D3D11 1
#define SDL_VIDEO_RENDER_D3D12 1
#define SDL_VIDEO_RENDER_GPU 1
/* #undef SDL_VIDEO_RENDER_METAL */
#define SDL_VIDEO_RENDER_VULKAN 1
#define SDL_VIDEO_RENDER_OGL 1
#define SDL_VIDEO_RENDER_OGL_ES2 1
/* #undef SDL_VIDEO_RENDER_PS2 */
/* #undef SDL_VIDEO_RENDER_PSP */
/* #undef SDL_VIDEO_RENDER_VITA_GXM */

/* Enable OpenGL support */
#define SDL_VIDEO_OPENGL 1
/* #undef SDL_VIDEO_OPENGL_ES */
#define SDL_VIDEO_OPENGL_ES2 1
/* #undef SDL_VIDEO_OPENGL_CGL */
/* #undef SDL_VIDEO_OPENGL_GLX */
#define SDL_VIDEO_OPENGL_WGL 1
#define SDL_VIDEO_OPENGL_EGL 1

/* Enable Vulkan support */
#define SDL_VIDEO_VULKAN 1

/* Enable Metal support */
/* #undef SDL_VIDEO_METAL */

/* Enable GPU support */
#define SDL_GPU_D3D11 1
#define SDL_GPU_D3D12 1
#define SDL_GPU_VULKAN 1
/* #undef SDL_GPU_METAL */

/* Enable system power support */
/* #undef SDL_POWER_ANDROID */
/* #undef SDL_POWER_LINUX */
#define SDL_POWER_WINDOWS 1
/* #undef SDL_POWER_MACOSX */
/* #undef SDL_POWER_UIKIT */
/* #undef SDL_POWER_HAIKU */
/* #undef SDL_POWER_EMSCRIPTEN */
/* #undef SDL_POWER_HARDWIRED */
/* #undef SDL_POWER_VITA */
/* #undef SDL_POWER_PSP */
/* #undef SDL_POWER_N3DS */

/* Enable system filesystem support */
/* #undef SDL_FILESYSTEM_ANDROID */
/* #undef SDL_FILESYSTEM_HAIKU */
/* #undef SDL_FILESYSTEM_COCOA */
/* #undef SDL_FILESYSTEM_DUMMY */
/* #undef SDL_FILESYSTEM_RISCOS */
/* #undef SDL_FILESYSTEM_UNIX */
#define SDL_FILESYSTEM_WINDOWS 1
/* #undef SDL_FILESYSTEM_EMSCRIPTEN */
/* #undef SDL_FILESYSTEM_VITA */
/* #undef SDL_FILESYSTEM_PSP */
/* #undef SDL_FILESYSTEM_PS2 */
/* #undef SDL_FILESYSTEM_N3DS */

/* Enable system storage support */
#define SDL_STORAGE_STEAM 1

/* Enable system FSops support */
/* #undef SDL_FSOPS_POSIX */
#define SDL_FSOPS_WINDOWS 1
/* #undef SDL_FSOPS_DUMMY */

/* Enable camera subsystem */
#define SDL_CAMERA_DRIVER_DUMMY 1
/* !!! FIXME: for later cmakedefine SDL_CAMERA_DRIVER_DISK 1 */
/* #undef SDL_CAMERA_DRIVER_V4L2 */
/* #undef SDL_CAMERA_DRIVER_COREMEDIA */
/* #undef SDL_CAMERA_DRIVER_ANDROID */
/* #undef SDL_CAMERA_DRIVER_EMSCRIPTEN */
#define SDL_CAMERA_DRIVER_MEDIAFOUNDATION 1
/* #undef SDL_CAMERA_DRIVER_PIPEWIRE */
/* #undef SDL_CAMERA_DRIVER_PIPEWIRE_DYNAMIC */
/* #undef SDL_CAMERA_DRIVER_VITA */

/* Enable dialog subsystem */
/* #undef SDL_DIALOG_DUMMY */

/* Enable assembly routines */
/* #undef SDL_ALTIVEC_BLITTERS */

/* Whether SDL_DYNAMIC_API needs dlopen */
/* #undef DYNAPI_NEEDS_DLOPEN */

/* Enable ime support */
/* #undef SDL_USE_IME */
/* #undef SDL_DISABLE_WINDOWS_IME */
/* #undef SDL_GDK_TEXTINPUT */

/* Platform specific definitions */
/* #undef SDL_IPHONE_KEYBOARD */
/* #undef SDL_IPHONE_LAUNCHSCREEN */

/* #undef SDL_VIDEO_VITA_PIB */
/* #undef SDL_VIDEO_VITA_PVR */
/* #undef SDL_VIDEO_VITA_PVR_OGL */

/* Libdecor version info */
#define SDL_LIBDECOR_VERSION_MAJOR 
#define SDL_LIBDECOR_VERSION_MINOR 
#define SDL_LIBDECOR_VERSION_PATCH 

#if !defined(HAVE_STDINT_H) && !defined(_STDINT_H_)
/* Most everything except Visual Studio 2008 and earlier has stdint.h now */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#ifndef _UINTPTR_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 uintptr_t;
#else
typedef unsigned int uintptr_t;
#endif
#endif
#endif /* Visual Studio 2008 */
#endif /* !_STDINT_H_ && !HAVE_STDINT_H */

/* Configure use of intrinsics */
/* #undef SDL_DISABLE_SSE */
/* #undef SDL_DISABLE_SSE2 */
/* #undef SDL_DISABLE_SSE3 */
/* #undef SDL_DISABLE_SSE4_1 */
/* #undef SDL_DISABLE_SSE4_2 */
/* #undef SDL_DISABLE_AVX */
/* #undef SDL_DISABLE_AVX2 */
/* #undef SDL_DISABLE_AVX512F */
#define SDL_DISABLE_MMX 1
#define SDL_DISABLE_LSX 1
#define SDL_DISABLE_LASX 1
#define SDL_DISABLE_NEON 1

#endif /* SDL_build_config_h_ */
