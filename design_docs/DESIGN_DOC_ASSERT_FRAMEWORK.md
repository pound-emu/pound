## **Design Document: Core Assertion Subsystem**

**Author:** GloriousTacoo, Lead Developer
**Status:** FINAL  
**Version:** 1.0  
**Date:** 2025-09-20  

*Disclaimer: This document was written by AI. I'm not a good technical writer.riter**

### **1. Problem Statement**

We require a rigorous, inescapable, and informative assertion framework that codifies our fail-fast philosophy. An invalid program state must lead to immediate, loud, and unrecoverable termination. There is no other acceptable outcome. This system will serve as the bedrock of the PVM's stability, ensuring that programmer errors are caught and exposed, not hidden.

### **2. Glossary**

*   **Assertion:** A predicate within the code that declares an invariant—a condition that must be true for the program to be considered correct. Its failure indicates a bug.
*   **Predicate:** The boolean expression evaluated by an assertion.
*   **Assertion Failure:** The event triggered when an assertion's predicate evaluates to false. This signifies a critical, non-recoverable programmer error.
*   **Fail-Fast:** The core design principle of this entire project. The system will not attempt to limp along in a corrupted state. At the first sign of an invalid condition, it will terminate.
*   **Unrecoverable Error:** Any state from which continued correct operation cannot be guaranteed. By definition, any assertion failure signals such a state.

### **3. Breaking Changes**

*   All usage of the standard library `<assert.h>` is hereby deprecated and forbidden.
*   A pre-commit hook will be integrated into the repository. It will scan for the token `assert(` and will reject any commit that contains it. This is not a suggestion. Your code will not be accepted if it uses the standard macro.
*   All existing modules must be migrated to the new `PVM_ASSERT` API. This is a one-time, mandatory refactoring effort. All pull requests will be blocked until this migration is complete.

### **4. Success Criteria**

*   **Adoption:** 100% of all precondition, postcondition, and invariant checks within the PVM codebase will utilize the new assertion framework. Zero exceptions.
*   **Information Richness:** A failed assertion must produce a diagnostic message on `stderr` containing, at minimum: the full text of the failed expression, the source file name, the line number, the enclosing function name, and an optional, developer-supplied formatted message.
*   **Inescapability:** The assertion framework must be impossible to disable. The state of the `NDEBUG` macro will have no effect. Assertions are a permanent, non-optional feature of the executable in all build configurations.
*   **Termination Guarantee:** A failed assertion must, without exception, result in immediate program termination via a call to `abort()`. No cleanup, no unwinding, no second chances. The process will stop *now*.

### **5. Proposed Design**

This framework is built upon an unyielding philosophy. You will internalize it.

*   **Tenet 1: Bugs Are Defects.** An assertion failure is not a runtime error. It is proof of a flaw in the logic of the code. It is a bug that you, the developer, introduced. The purpose of this system is to expose these flaws.
*   **Tenet 2: Failure is Absolute.** There is no "graceful" way to handle a broken invariant. The only correct action is immediate termination to prevent the propagation of a corrupt state. The output must be loud, clear, and provide maximum context to diagnose the defect.
*   **Tenet 3: Correctness is Not a "Debug" Feature.** Assertions are always on. They are an integral part of the executable's logic and its contract for safe operation. Any performance argument against this is invalid and will be dismissed. The cost of a passing check is infinitesimal; the cost of a latent bug is mission failure.
*   **Tenet 4: Clarity is Mandatory.** The API will be simple, but its use requires discipline. You will provide context in your assertions. A naked expression is often not enough to explain *why* a condition must be true.

The API will consist of three macros. Use them correctly.

`PVM_ASSERT(expression)`  
`PVM_ASSERT_MSG(expression, fmt, ...)`  
`PVM_UNREACHABLE()`

### **6. Technical Design**

The implementation will be brutally simple and effective.

1.  **Frontend Macros (`common/assert.h`):**
    *   These macros are the complete public API. There are no other entry points.
    *   `PVM_ASSERT(expression)`: Expands to an `if` statement. If `(expression)` is false, it calls the internal failure handler, passing the stringified expression (`#expression`), file, line, and function name.
        ```c
        do {
            if (!(expression)) {
                pound_internal_assert_fail(__FILE__, __LINE__, __func__, #expression, NULL);
            }
        } while(0)
        ```
    *   `PVM_ASSERT_MSG(expression, fmt, ...)`: Similar to the above, but if the check fails, it first formats the developer-supplied message into a temporary buffer and passes that buffer to the failure handler. The formatting cost is **only** paid on failure. There is no excuse for not using this macro to provide context for complex invariants.
    *   `PVM_UNREACHABLE()`: This is not a suggestion. It is a declaration that a code path is logically impossible. It expands to a direct call to the failure handler with a static message like "Unreachable code executed". If this assertion ever fires, the logical model of the function is wrong.

2.  **Failure Handler (`common/assert.c`):**
    *   A single, internal C function: `void pound_internal_assert_fail(const char* file, int line, const char* func, const char* expr_str, const char* user_msg)`.
    *   This function is marked with `_Noreturn` (or `__attribute__((noreturn))`). It will never return to the caller. The compiler will know this.
    *   It will lock a mutex to prevent garbled output if two threads fail simultaneously (a near-impossibility, but we engineer for correctness).
    *   It will format a single, comprehensive, multi-line error message to `stderr`. The format is fixed and not subject to debate:
        ```
        ================================================================================
        PVM ASSERTION FAILURE
        ================================================================================
        File:         src/kvm/mmu.cpp
        Line:         521
        Function:     mmu_translate_va
        Expression:   page_table->entry[idx].is_valid()
        Message:      Attempted to translate VA 0xDEADBEEF but page table entry was invalid.
        ================================================================================
        Terminating program via abort(). Core dump expected.
        ```
    *   After printing, it will immediately call `abort()`.

### **7. Components**

*   **Application Modules (kvm, frontend, etc.):** These are the components whose logic is being enforced. They will include `common/assert.h` and use the macros to state invariants.
*   **Assertion Core (`common/assert`):** This small, self-contained module provides the macros and the single failure handler function. It has no purpose other than to enforce correctness and terminate the program.
*   **Build System:** Will be configured to enforce the ban on `<assert.h>`.

### **8. Dependencies**

*   **C Standard Library:** For `fprintf`, `stderr`, and `abort`.

### **9. Major Risks & Mitigations**

*   **Risk 1: Performance Complacency.** A developer may write a computationally expensive operation inside an assertion's predicate.
    *   **Mitigation:** This is a failure of code review, not a failure of the framework. The performance of a *passing* assertion (a boolean check) is negligible and is the accepted cost of safety. The performance of a *failing* assertion is irrelevant, as the program ceases to exist. Code reviewers are directed to reject any assertion that performs non-trivial work. Assertions check state; they do not compute it.
*   **Risk 2: Confusion with Error Handling.** A developer might use `PVM_ASSERT` to handle recoverable, runtime errors (e.g., failed I/O, invalid user input).
    *   **Mitigation:** Documentation and merciless code review. This will be made explicitly clear: **Assertions are for bugs.** They are for conditions that, if false, prove the program's logic is flawed. Runtime errors must be handled through status codes or other proper error-handling mechanisms. Any pull request that misuses assertions for error handling will be rejected immediately.

### **10. Out of Scope**

*   **Error Recovery:** The word "recovery" does not apply to an assertion failure. It is, by definition, an unrecoverable state.
*   **Crash Reporting Infrastructure:** This framework's responsibility ends at calling `abort()`. The generation of a core dump and any subsequent analysis is the responsibility of the execution environment, not this library. We provide the trigger; other systems can handle the post-mortem.
*   **Any Form of "Gentle" Shutdown:** Resources will not be freed. Files will not be flushed. Sockets will not be closed. Such actions would be performed by a program in a corrupt state and cannot be trusted. `abort()` is the only clean exit.

### **12. Alternatives Considered**

*   **Alternative #1: Use standard `assert.h` and globally `#undef NDEBUG`.**
    *   **Pros:** Requires no new code.
    *   **Cons:** The output is spartan and implementation-defined. It provides no mechanism for custom, formatted messages. It creates a dependency on a build flag that could be accidentally overridden by a sub-project or complex build configuration. It is fragile.
    *   **Reasons Discarded:** Insufficiently informative and not robust enough for Pound. We will control our own destiny, not hope a build flag is set correctly.

*   **Alternative #2: Ad-hoc `if (condition) { fprintf(...); abort(); }`.**
    *   **Pros:** None. This is engineering malpractice.
    *   **Cons:** Verbose, error-prone, guarantees inconsistent output formats, and omits critical context like the stringified expression and function name unless manually added each time. It is a recipe for unmaintainable chaos.
    *   **Reasons Discarded:** This is not a real alternative. It is an anti-pattern that this framework is designed to eradicate.

### **13. Appendix**
#### **Example Usage and Output**

**Code:**
```c
// in some function in memory_manager.c
void* memory_manager_alloc(struct mmu* mmu, size_t bytes) {
    PVM_ASSERT_MSG(mmu != NULL, "MMU context must not be null.");
    PVM_ASSERT_MSG(bytes > 0 && bytes < MAX_ALLOC_SIZE, "Invalid allocation size requested: %zu bytes", bytes);
    
    // ... logic ...
    
    // This case should be handled by prior logic
    if (page_is_full) {
        PVM_UNREACHABLE();
    }
    
    return ptr;
}
```

**Sample output from a failed assertion:**
```
================================================================================
PVM ASSERTION FAILURE
================================================================================
File:         src/core/memory_manager.c
Line:         84
Function:     memory_manager_alloc
Expression:   bytes > 0 && bytes < MAX_ALLOC_SIZE
Message:      Invalid allocation size requested: 0 bytes
================================================================================
Terminating program via abort().
