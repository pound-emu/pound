// Copyright 2025 Xenon Emulator Project. All rights reserved.

#include "Base/Assert.h"

#include "Filter.h"

namespace Base {
namespace Log {

template <typename It>
Level GetLevelByName(const It begin, const It end) {
  for (u8 i = 0; i < static_cast<u8>(Level::Count); ++i) {
    const char* level_name = GetLevelName(static_cast<Level>(i));
    if (std::string_view(begin, end).compare(level_name) == 0) {
      return static_cast<Level>(i);
    }
  }
  return Level::Count;
}

template <typename It>
Class GetClassByName(const It begin, const It end) {
  for (u8 i = 0; i < static_cast<u8>(Class::Count); ++i) {
    const char* level_name = GetLogClassName(static_cast<Class>(i));
    if (std::string_view(begin, end).compare(level_name) == 0) {
      return static_cast<Class>(i);
    }
  }
  return Class::Count;
}

template <typename Iterator>
bool ParseFilterRule(Filter &instance, Iterator begin, Iterator end) {
  const auto levelSeparator = std::find(begin, end, ':');
  if (levelSeparator == end) {
    LOG_ERROR(Log, "Invalid log filter. Must specify a log level after `:`: {}", std::string_view(begin, end));
    return false;
  }

  const Level level = GetLevelByName(levelSeparator + 1, end);
  if (level == Level::Count) {
    LOG_ERROR(Log, "Unknown log level in filter: {}", std::string_view(begin, end));
    return false;
  }

  if (std::string_view(begin, levelSeparator).compare("*") == 0) {
    instance.ResetAll(level);
    return true;
  }

  const Class logClass = GetClassByName(begin, levelSeparator);
  if (logClass == Class::Count) {
    LOG_ERROR(Log, "Unknown log class in filter: {}", std::string(begin, end));
    return false;
  }

  instance.SetClassLevel(logClass, level);
  return true;
}

/// Macro listing all log classes. Code should define CLS and SUB as desired before invoking this.
#define ALL_LOG_CLASSES()                                                                        \
  CLS(Log)                                                                                       \
  CLS(Base)                                                                                      \
  SUB(Base, Filesystem)                                                                          \
  CLS(Config)                                                                                    \
  CLS(Debug)                                                                                     \
  CLS(System)                                                                                    \
  CLS(Render)                                                                                    \
  CLS(ARM)                                                                                       \
  CLS(Memory)                                                                                    \

// GetClassName is a macro defined by Windows.h, grrr...
const char* GetLogClassName(Class logClass) {
  switch (logClass) {
#define CLS(x)                                                                                   \
  case Class::x:                                                                                 \
    return #x;
#define SUB(x, y)                                                                                \
  case Class::x##_##y:                                                                           \
    return #x "." #y;
    ALL_LOG_CLASSES()
#undef CLS
#undef SUB
  case Class::Count:
  default:
    break;
  }
  UNREACHABLE();
}

const char* GetLevelName(Level logLevel) {
#define LVL(x)                                                                                   \
  case Level::x:                                                                                 \
    return #x
  switch (logLevel) {
  LVL(Trace);
  LVL(Debug);
  LVL(Info);
  LVL(Warning);
  LVL(Error);
  LVL(Critical);
  LVL(Guest);
  case Level::Count:
  default:
    break;
  }
#undef LVL
  UNREACHABLE();
}

Filter::Filter(Level defaultLevel) {
  ResetAll(defaultLevel);
}

void Filter::ResetAll(Level level) {
  classLevels.fill(level);
}

void Filter::SetClassLevel(Class logClass, Level level) {
  classLevels[static_cast<size_t>(logClass)] = level;
}

void Filter::ParseFilterString(const std::string_view &filterView) {
  auto clause_begin = filterView.cbegin();
  while (clause_begin != filterView.cend()) {
    auto clause_end = std::find(clause_begin, filterView.cend(), ' ');

    // If clause isn't empty
    if (clause_end != clause_begin) {
      ParseFilterRule(*this, clause_begin, clause_end);
    }

    if (clause_end != filterView.cend()) {
      // Skip over the whitespace
      ++clause_end;
    }
    clause_begin = clause_end;
  }
}

bool Filter::CheckMessage(Class logClass, Level level) const {
  return static_cast<u8>(level) >=
         static_cast<u8>(classLevels[static_cast<size_t>(logClass)]);
}

bool Filter::IsDebug() const {
  return std::any_of(classLevels.begin(), classLevels.end(), [](const Level& l) {
    return static_cast<u8>(l) <= static_cast<u8>(Level::Debug);
  });
}

} // namespace Log
} // namespace Base
