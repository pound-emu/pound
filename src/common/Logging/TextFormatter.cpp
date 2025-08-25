// Copyright 2025 Xenon Emulator Project. All rights reserved.

#include "common/Assert.h"
#include "common/Config.h"

#include "TextFormatter.h"

#include "Filter.h"
#include "LogEntry.h"

namespace Base {
namespace Log {

std::string FormatLogMessage(const Entry &entry) {
  const char *className = GetLogClassName(entry.logClass);
  const char *levelName = GetLevelName(entry.logLevel);

  if (Config::isLogAdvanced() && entry.filename) {
    return fmt::format("[{}] <{}> {}:{}:{}: {}", className, levelName, entry.filename,
      entry.function, entry.lineNum, entry.message);
  } else {
    return fmt::format("[{}] <{}> {}", className, levelName, entry.message);
  }
}

#define ESC "\x1b"
void PrintMessage(const std::string &color, const Entry &entry) {
  std::string msg = entry.formatted ? FormatLogMessage(entry) : entry.message;
  const std::string str = color + msg.append(ESC "[0m") + (entry.formatted ? "\n" : "");
  fputs(str.c_str(), stdout);
}

void PrintColoredMessage(const Entry &entry) {
  // NOTE: Custom colors can be achieved
  // std::format("\x1b[{};2;{};{};{}m", color.bg ? 48 : 38, color.r, color.g, color.b)
  const char *color = "";
  switch (entry.logLevel) {
  case Level::Trace: // Grey
    color = ESC "[1;30m";
    break;
  case Level::Debug: // Cyan
    color = ESC "[0;36m";
    break;
  case Level::Info: // Bright gray
    color = ESC "[0;37m";
    break;
  case Level::Warning: // Bright yellow
    color = ESC "[1;33m";
    break;
  case Level::Error: // Bright red
    color = ESC "[1;31m";
    break;
  case Level::Critical: // Bright magenta
    color = ESC "[1;35m";
    break;
  case Level::Guest: // Green
    color = ESC "[0;92m";
    break;
  case Level::Count:
    UNREACHABLE();
  }

  PrintMessage(color, entry);
}
#undef ESC

} // namespace Log
} // namespace Base
