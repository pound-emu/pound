// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#include <string>
#include <string_view>

namespace Base {
namespace Log {

struct Entry;

/// Formats a log entry into the provided text buffer.
std::string FormatLogMessage(const Entry &entry);

/// Formats and prints a log entry to stderr.
void PrintMessage(const std::string &color, const Entry &entry);

/// Prints the same message as `PrintMessage`, but colored according to the severity level.
void PrintColoredMessage(const Entry &entry);

} // namespace Log
} // namespace Base
