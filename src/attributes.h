#ifndef POUND_ATTRIBUTES_H
#define POUND_ATTRIBUTES_H

/// POUND_HOT()/POUND_COLD()
/// Marks a function as hot or cold. Hot makes the compiler optimize it more
/// aggressively. Cold marks the function as rarely executed.
///
/// Usage:
/// POUND_HOT bal_error_t emit_instruction(...);
#if POUND_COMPILER_GCC

#define POUND_HOT  __attribute__((hot))
#define POUND_COLD __attribute__((cold))

#else

#define POUND_HOT
#define POUND_COLD

#endif

/// POUND_LIKELY(x)/POUND_UNLIKELY(x)
/// Hints to the CPU branch predictor. Should only be used in hot functions.
///
/// Usage: if (POUND_UNLIKELY(ptr == NULL)) { ... }
#if POUND_COMPILER_GCC

#define POUND_LIKELY(x)   __builtin_expect(!!(x), 1)
#define POUND_UNLIKELY(x) __builtin_expect(!!(x), 0)

#else

#define POUND_LIKELY(x)   (x)
#define POUND_UNLIKELY(x) (x)

#endif

/// POUND_ALIGNED(x)
/// Aligns a variable or a structure to x bytes.
///
/// Usage: POUND_ALIGNED(64)  struct data { ... };
#if POUND_COMPILER_GCC

#define POUND_ALIGNED(x) __attribute__((aligned(x)))

#elif POUND_COMPILER_MSVC

#define POUND_ALIGNED(x) __declspec(align(x))

#else

#define POUND_ALIGNED(x)

#endif

/// POUND_RESTRICT
/// Tells the compiler that a pointer does not alias any other pointer in
/// current scope.
#if POUND_COMPILER_GCC

#define POUND_RESTRICT __restrict__

#elif POUND_COMPILER_MSVC

#define POUND_RESTRICT __restrict

#else

#define POUND_RESTRICT

#endif

#endif // POUND_ATTRIBUTES_H

/*** end of file ***/
