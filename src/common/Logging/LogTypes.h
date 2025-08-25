// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#include "common/Types.h"

namespace Base {
namespace Log {

/// Specifies the severity or level of detail of the log message
enum class Level : const u8 {
  Trace,      ///< Extremely detailed and repetitive debugging information that is likely to pollute logs
  Debug,      ///< Less detailed debugging information
  Info,       ///< Status information from important points during execution
  Warning,    ///< Minor or potential problems found during execution of a task
  Error,      ///< Major problems found during execution of a task that prevent it from being completed
  Critical,   ///< Major problems during execution that threaten the stability of the entire application
  Guest,      ///< Output from the guest system
  Count       ///< Total number of logging levels
};

/*
 * Specifies the sub-system that generated the log message
 *
 * @note If you add a new entry here, also add a corresponding one to `ALL_LOG_CLASSES` in
 * filtercpp
 */
enum class Class : const u8 {
  Log,                    // Messages about the log system itself
  Base,                   // System base routines: FS, logging, etc
  Base_Filesystem,        // Filesystem messages
  Config,                 // Emulator configuration (including commandline)
  Debug,                  // Debugging tools
  System,                 // Base System messages
  Render,                 // OpenGL and Window messages
  ARM,
  PROBE,
  MMIO_S1,
  Memory,
  Count                   // Total number of logging classes
};

} // namespace Log
} // namespace Base
