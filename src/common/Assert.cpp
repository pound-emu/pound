// Copyright 2025 Xenon Emulator Project. All rights reserved.

#include <stdio.h>

#include "Arch.h"
#include "Assert.h"
#include "Logging/Backend.h"

#ifdef _MSC_VER
#define Crash() __debugbreak()
#else
#if defined(ARCH_X86_64)
#define Crash() __asm__ __volatile__("int $3")
#elif defined(ARCH_X86)
#define Crash() __asm__ __volatile__("int $3")
#elif defined(ARCH_AARCH64)
#define Crash() __asm__ __volatile__("brk 0")
#elif defined(ARCH_PPC)
#include <signal.h>
#ifdef SIGTRAP
#define Crash() raise(SIGTRAP)
#else
#define Crash() raise(SIGABRT)
#endif
#else
#define Crash() __builtin_trap()
#endif
#endif  // _MSVC_VER

void throw_fail_impl()
{
    ::fflush(stdout);
    Crash();
}

void assert_fail_impl()
{
    printf("Assertion Failed!\n");
    throw_fail_impl();
}

[[noreturn]] void unreachable_impl()
{
    ::fflush(stdout);
    Crash();
    throw std::runtime_error("Unreachable code");
}

void assert_fail_debug_msg(const std::string& msg)
{
    printf("Assertion Failed! %s\n", msg.c_str());
    assert_fail_impl();
}

void throw_fail_debug_msg(const std::string& msg)
{
    printf("Assertion Failed! %s\n", msg.c_str());
    throw_fail_impl();
}
