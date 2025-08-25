// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#include <fmt/format.h>

#include "Arch.h"

// Signed
using s8 = signed char;
using s16 = signed short;
using s32 = signed int;
using sl32 = signed long;
using s64 = signed long long;

// Unsigned
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using ul32 = unsigned long;
using u64 = unsigned long long;

using uptr = uintptr_t;

// Floating point
using f32 = float;
using f64 = double;

// Function pointer
template <typename T>
requires std::is_function_v<T> using fptr = std::add_pointer_t<T>;

// UDLs for memory size values
[[nodiscard]] constexpr u64 operator""_KB(const u64 x)
{
    return 1000ULL * x;
}
[[nodiscard]] constexpr u64 operator""_KiB(const u64 x)
{
    return 1024ULL * x;
}
[[nodiscard]] constexpr u64 operator""_MB(const u64 x)
{
    return 1000_KB * x;
}
[[nodiscard]] constexpr u64 operator""_MiB(const u64 x)
{
    return 1024_KiB * x;
}
[[nodiscard]] constexpr u64 operator""_GB(const u64 x)
{
    return 1000_MB * x;
}
[[nodiscard]] constexpr u64 operator""_GiB(const u64 x)
{
    return 1024_MiB * x;
}
