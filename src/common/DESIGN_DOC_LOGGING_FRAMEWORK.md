## **Design Document: Core Logging Subsystem**

**Author:** GloriousTaco, Lead Developer  
**Status:** FINAL  
**Version:** 1.0  
**Date:** 2025-09-14  

*Disclaimer: This was mostly written with AI. I'm not a good technical writer*

### **1. Problem Statement**

The Pound project's current logging system is full of object oriented abstractions with no documentation. The system currently risides in `src/common/Logging` with no one going anywhere near it. However, as we move on from prototyping to testing, we require a logging framework that provides a performant diagnostic output and is easy to maintain.

### **2. Glossary**

*   **Log Level:** A classification of a log message's severity (e.g., TRACE, DEBUG, INFO, WARN, ERROR, FATAL).
*   **Log Sink:** A destination for log messages. This can be a console, a file, a network socket, etc.
*   **Structured Logging:** A practice of logging messages in a consistent, machine-readable format (e.g., JSON), with key-value pairs, rather than unstructured text strings.
*   **Compile-Time Log Level:** The minimum log level that will be compiled into the binary. Messages below this level are completely removed by the preprocessor, incurring zero runtime cost.
*   **Runtime Log Level:** The minimum log level that the system will process and output at runtime. This can be changed dynamically without recompiling.
*   **PVM:** Pound Virtual Machine, the overall project.

### **3. Breaking Changes**

*   This design will deprecate and forbid all usage of `printf`, `fprintf(stderr, ...)` and other direct-to-console I/O for diagnostic purposes within the PVM codebase (excluding `main.cpp` for initial setup/teardown).
*   A pre-commit hook will be introduced to enforce this policy, which will cause existing pull requests to fail until they are updated to use the new logging API.
*   All existing modules will require modification to adopt the new logging API.

### **4. Success Criteria**

*   **Adoption:** 100% of diagnostic messages in the `kvm`, `common`, `host`, and `frontend` modules will use the new logging system.
*   **Performance:** In a release build with the runtime log level set to `INFO`, the overhead of disabled `DEBUG` and `TRACE` log statements shall be statistically unmeasurable (<0.1% performance impact) compared to a binary compiled with logging completely disabled.
*   **Usability:** A developer can successfully add a new namespaced log message and filter the system output to show logs only from their module within 15 minutes, using only the API header and a quick-start guide.

### **5. Proposed Design**

We will implement a lightweight, header-only, macro-based logging framework heavily inspired by systems like `spdlog` but simplified for our specific needs. The core design is built on the following tenets:

*   **Tenet 1: Performance is Paramount.** Logging is a diagnostic tool; it must never be the cause of a performance issue. The system will aggressively optimize away disabled log calls at compile time.
*   **Tenet 2: Structure is Mandatory.** All log messages will be structured, capturing not just a message but also the severity level, timestamp, source location (file and line), and module.
*   **Tenet 3: Control is Granular.** Developers must have fine-grained control over logging verbosity at both compile time and runtime, on a per-module basis.
*   **Tenet 4: Simplicity in Use.** The API presented to developers must be simple and intuitive, encouraging adoption through macros like `LOG_WARN(...)`.

The primary user interface will be a set of macros:  
`LOG_TRACE(module, fmt, ...)`  
`LOG_DEBUG(module, fmt, ...)`  
`LOG_INFO(module, fmt, ...)`  
`LOG_WARN(module, fmt, ...)`  
`LOG_ERROR(module, fmt, ...)`  
`LOG_FATAL(module, fmt, ...)`  

### **6. Technical Design**

The system will be composed of three main parts: the frontend macros, the logging core, and the output sink.

1.  **Frontend Macros:**
    *   The `LOG_X` macros will be the only public-facing API.
    * the `LOG_FATAL` macro will be terminal. After logging the message, it will immediately terminate the program via a call to `abort()`. 
    *   The Log macros will first check against a `COMPILE_TIME_LOG_LEVEL`. If the message level is below this threshold, the macro will expand to nothing (`(void)0`), ensuring the code and its arguments are completely compiled out.
    *   If the level is sufficient, the macro will expand into a call to a logging core function, automatically passing `__FILE__`, `__LINE__`, the log level, and the module name.
    *   This will live in a `common/logging.h` header.

2.  **Logging Core:**
    *   A central `logger_log()` function will be the target of the macros.
    *   This function will check the message's log level against a globally configured `runtime_log_level` for the specified module. If the level is insufficient, the function returns immediately.
    *   If the level is sufficient, it will capture the current high-resolution timestamp, format the message string using the `fmt` library (which we already have as a dependency), and pass the formatted output string to the active sink.
    *   A small utility will manage the runtime log levels for each registered module (e.g., `logger_set_level("kvm", LEVEL_TRACE)`).

3.  **Output Sink:**
    *   The default sink will be a thread-safe, mutex-protected object that writes to `stderr`.
    *   The output format will be structured and non-negotiable: `[ISO8601 Timestamp] [LEVEL] [module] [file:line] Message`
    *   Example: `[2025-09-14T11:23:45.1234Z] [ERROR] [kvm] [mmu.cpp:412] Page table fault at GPA 0xDEADBEEF: Invalid permissions.`
    *   The design will allow for the possibility of replacing this sink in the future (e.g., to log to a file), but the initial implementation will be `stderr` only.

### **7. Components**

*   **Application Modules (kvm, frontend, etc.):** These are the *producers* of log messages. They will include `common/logging.h` and use the `LOG_X` macros.
*   **Logging Core (`common/logging`):** The central library responsible for filtering, formatting, and dispatching log messages.
*   **Sink (`common/logging`):** The *consumer* of formatted log messages. Initially, this is the `stderr` writer.
*   **Main Application (`main.cpp`):** The owner of the system. It is responsible for initializing the logging system, setting initial runtime log levels (e.g., from command-line arguments), and shutting it down cleanly.

### **8. Dependencies**

*   **`fmt` library:** Will be used for high-performance string formatting. This is already a project dependency.
*   **C++ Standard Library:** Specifically `<chrono>` for timestamps and `<mutex>` for thread safety in the sink.

### **9. Major Risks & Mitigations**

*   **Risk 1: Performance Overhead.** Careless implementation could lead to significant overhead even for enabled logs (e.g., excessive string allocation, slow timestamping).
    *   **Mitigation:** The use of the `fmt` library is a known high-performance choice. We will benchmark the logging of 1 million messages in a tight loop to quantify the overhead and ensure it meets the success criteria.
*   **Risk 2: Thread Safety Issues.** Improper locking in the sink could lead to garbled output or race conditions when multiple threads log simultaneously.
    *   **Mitigation:** A single `std::mutex` will protect all writes to the sink. Code will be peer-reviewed specifically for thread-safety concerns.
*   **Risk 3: Slow Adoption / Incorrect Usage.** Developers may continue to use `printf` out of habit.
    *   **Mitigation:** The API will be designed for extreme ease of use. A short, clear guide will be written. Critically, a pre-commit hook and CI linting job will be added to the build system to automatically fail any code that uses `printf`/`fprintf`. This makes the correct path the only path.

### **10. Out of Scope**

*   **File-based Logging:** The initial version will only log to `stderr`. A file sink is a desirable future feature but is not required for V1.
*   **Network Logging:** Transmitting logs over a network is not a requirement.
*   **Log Rotation:** Since we are not logging to files, rotation is not applicable.
*   **Dynamic Sink Swapping:** The sink will be configured at startup and fixed for the application's lifetime.

### **12. Alternatives Considered**

*   **Alternative #1: Use a full-featured third-party library (e.g., `spdlog`, `glog`).**
    *   **Pros:** Mature, feature-rich, well-tested.
    *   **Cons:** Adds another third-party dependency to maintain. May contain features (async logging, complex file sinks) that add unnecessary complexity and code size for our specific use case. Our needs are simple enough that a minimal, custom implementation is justified.
    *   **Reasons Discarded:** The primary reason is to minimize external dependencies and code footprint. We can achieve 95% of the value with 10% of the complexity by building a minimal system tailored to our exact needs, leveraging our existing `fmt` dependency.

*   **Alternative #2: "Do Nothing" (Continue using `printf`).**
    *   **Pros:** No development effort required.
    *   **Cons:** Unstructured, impossible to filter, no severity levels, no timestamps, not thread-safe. Fails to meet every requirement for a mission-safe diagnostic system.
    *   **Reasons Discarded:** This is a non-starter. It is fundamentally unsuitable for the project's goals.

### **13. Appendix**
#### **Benchmarking Performance**
1.  **Baseline Measurement (A):** A simple `for` loop will be created that iterates 1 million times, performing a trivial, non-optimizable operation (e.g., incrementing a `volatile` integer). The total execution time of this loop will be measured using a high-resolution clock.
2.  **Disabled Log Measurement (B):** The same loop will be modified to include a `LOG_DEBUG` call. The test binary will be compiled with a `COMPILE_TIME_LOG_LEVEL` of `INFO`. This means the `LOG_DEBUG` macro will expand to `(void)0` and be completely compiled out. The execution time of this loop will be measured.
3.  **Enabled Log Measurement (C):** The same loop will be run, but with the `runtime_log_level` set to allow the `LOG_DEBUG` messages to be processed and written to the sink. The sink's output will be redirected to `/dev/null` to measure the cost of formatting and dispatch, not the I/O cost of the terminal itself. The execution time will be measured.

**Analysis:**
*   The difference between **(B)** and **(A)** should be zero or statistically insignificant, proving the success criterion that disabled logs have no overhead.
*   The difference between **(C)** and **(A)** represents the full overhead of an enabled log call. This allows us to calculate the average cost-per-message on our specific hardware.

