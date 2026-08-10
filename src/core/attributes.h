//! Compiler specific attribute macros.

#ifndef POUND_ATTRIBUTES_H
#define POUND_ATTRIBUTES_H

/// POUND_HOT()/POUND_COLD()
/// Marks a function as hot or cold. Hot makes the compiler optimize it more
/// aggressively. Cold marks the function as rarely executed.
///
/// Usage:
/// POUND_HOT POUND_error_t emit_instruction(...);
#define POUND_HOT  __attribute__((hot))
#define POUND_COLD __attribute__((cold))

/// POUND_LIKELY(x)/POUND_UNLIKELY(x)
/// Hints to the CPU branch predictor. Should only be used in hot functions.
///
/// Usage: if (POUND_UNLIKELY(ptr == NULL)) { ... }
#define POUND_LIKELY(x)   __builtin_expect(!!(x), 1)
#define POUND_UNLIKELY(x) __builtin_expect(!!(x), 0)

/// POUND_ALIGNED(x)
/// Aligns a variable or a structure to x bytes.
///
/// Usage: POUND_ALIGNED(64)  struct data { ... };
#define POUND_ALIGNED(x) __attribute__((aligned(x)))

/// POUND_RESTRICT
/// Tells the compiler that a pointer does not alias any other pointer in
/// current scope.
#define POUND_RESTRICT __restrict__

/// POUND_EXPORT
/// Marks a function or global variable for export.
///
/// Usage: POUND_EXPORT void POUND_public_api_function(void);
#define POUND_EXPORT __attribute__((visibility("default")))

/// POUND_WEAK
/// Marks a function or variable as weak, allowing the host application to override it at link-time
/// by providing a strong symbol with the same name.
///
/// Usage: POUND_WEAK void POUND_default_logger(...);
#define POUND_WEAK __attribute__((weak))

/// POUND_THREAD_LOCAL
/// Declares a variable with thread-local storage duration.
#define POUND_THREAD_LOCAL _Thread_local

/// POUND_NOINLINE
/// Prevent function inlining.
///
/// Usage: POUND_NOINLINE void my_function();
#define POUND_NOINLINE __attribute__((noinline))

/*** end of file ***/
