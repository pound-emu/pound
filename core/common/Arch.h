// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_X86_64 1
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_AARCH64 1
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
    defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
#define ARCH_PPC 1
#endif
