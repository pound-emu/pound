// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#include <chrono>

#include "LogTypes.h"

namespace Base {
namespace Log {

/*
 * A log entry. Log entries are store in a structured format to permit more varied output
 * formatting on different frontends, as well as facilitating filtering and aggregation.
 */
struct Entry {
  std::chrono::microseconds timestamp = {};
  Class logClass = {};
  Level logLevel = {};
  const char *filename = nullptr;
  u32 lineNum = 0;
  std::string function = {};
  std::string message = {};
  bool formatted = true;
};

} // namespace Log
} // namespace Base
