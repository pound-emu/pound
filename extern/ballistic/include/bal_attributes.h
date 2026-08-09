//! Compiler specific attribute macros.

#ifndef BALLISTIC_ATTRIBUTES_H
#define BALLISTIC_ATTRIBUTES_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// BAL_HOT()/BAL_COLD()
/// Marks a function as hot or cold. Hot makes the compiler optimize it more
/// aggressively. Cold marks the function as rarely executed.
///
/// Usage:
/// BAL_HOT bal_error_t emit_instruction(...);
#define BAL_HOT  __attribute__((hot))
#define BAL_COLD __attribute__((cold))

/// BAL_LIKELY(x)/BAL_UNLIKELY(x)
/// Hints to the CPU branch predictor. Should only be used in hot functions.
///
/// Usage: if (BAL_UNLIKELY(ptr == NULL)) { ... }
#define BAL_LIKELY(x)   __builtin_expect(!!(x), 1)
#define BAL_UNLIKELY(x) __builtin_expect(!!(x), 0)

/// BAL_ALIGNED(x)
/// Aligns a variable or a structure to x bytes.
///
/// Usage: BAL_ALIGNED(64)  struct data { ... };
#define BAL_ALIGNED(x) __attribute__((aligned(x)))

/// BAL_RESTRICT
/// Tells the compiler that a pointer does not alias any other pointer in
/// current scope.
#define BAL_RESTRICT __restrict__

/// BAL_EXPORT
/// Marks a function or global variable for export.
///
/// Usage: BAL_EXPORT void bal_public_api_function(void);
#define BAL_EXPORT __attribute__((visibility("default")))

/// BAL_WEAK
/// Marks a function or variable as weak, allowing the host application to override it at link-time
/// by providing a strong symbol with the same name.
///
/// Usage: BAL_WEAK void bal_default_logger(...);
#define BAL_WEAK __attribute__((weak))

/// BAL_THREAD_LOCAL
/// Declares a variable with thread-local storage duration.
#define BAL_THREAD_LOCAL _Thread_local

/// BAL_NOINLINE
/// Prevent function inlining.
///
/// Usage: BAL_NOINLINE void my_function();
#define BAL_NOINLINE __attribute__((noinline))

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BALLISTIC_ATTRIBUTES_H

/*** end of file ***/
