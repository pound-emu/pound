// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#include <string>

namespace Base
{

// Like GetLastErrorMsg(), but passing an explicit error code.
// Defined in error.cpp.
[[nodiscard]] std::string NativeErrorToString(const s32 e);

// Generic function to get last error message.
// Call directly after the command or use the error num.
// This function might change the error code.
// Defined in error.cpp.
[[nodiscard]] std::string GetLastErrorMsg();

}  // namespace Base
