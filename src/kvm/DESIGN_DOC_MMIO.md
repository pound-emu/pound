## **Design Document: MMIO Subsystem**

**Author:** GloriousTacoo, Lead Developer  
**Status:** FINAL  
**Version:** 1.0  
**Date:** 2025-09-20  

*Disclaimer: This document was mostly written by AI. I'm not a good technical writer.*

### **1. Problem Statement**

A virtual machine is not merely a CPU core; it is a system. A system includes peripherals. In modern architectures, communication with these peripherals is overwhelmingly achieved via Memory-Mapped I/O (MMIO). A guest operating system's attempt to read from or write to a specific physical memory address is not a memory access, but a command to a virtual device.

The MMIO subsystem is the central nervous system for this communication. It is the gatekeeper that intercepts these special memory accesses and dispatches them to the correct emulated device handlers. Its failure is not an option. A slow, buggy, or poorly designed MMIO dispatcher results in a slow, buggy, and fundamentally incorrect virtual machine.

We require an MMIO dispatcher that is:
1.  **Fast:** It sits on the most critical path of the PVM—memory access. Its overhead must be minimal to the point of being statistically insignificant for non-MMIO accesses.
2.  **Correct:** It must unambiguously and correctly route every access to the one and only correct handler, or correctly identify it as a non-MMIO access.
3.  **Rigid:** The registration process for MMIO regions must be strict, preventing common errors such as overlapping address spaces at system configuration time, not at runtime.
4.  **Testable:** The entire subsystem must be verifiable in isolation, independent of the full PVM, to prove its correctness.

### **2. Glossary**

*   **MMIO (Memory-Mapped I/O):** The use of a shared memory address space for both main memory and peripheral device registers.
*   **Guest Physical Address (GPA):** The physical memory address as seen from within the guest virtual machine. This is the address space the MMIO subsystem operates on.
*   **MMIO Region:** A contiguous, half-open range of GPAs, `[base, end)`, assigned to a single virtual device.
*   **MMIO Handler:** A function pointer (or pair of function pointers for read/write) that implements the behavior of a virtual device when its MMIO region is accessed.
*   **Dispatcher:** The core logic responsible for taking an incoming GPA and efficiently finding the corresponding MMIO handler.
*   **MMIO Database (`mmio_db_t`):** The central data structure holding all registered MMIO regions and their associated handlers.
*   **Structure of Arrays (SoA):** The chosen data layout for the MMIO database. Parallel arrays (`handlers`, `address_ranges`) are used instead of a single array of complex structs. This improves cache performance by ensuring memory accesses during the dispatch search are contiguous.

### **3. Breaking Changes**

*   Any and all previous, ad-hoc mechanisms for device emulation are deprecated and will be removed.
*   All virtual devices (e.g., UART, interrupt controller, timers) **must** register their memory regions through the `mmio_db_register` function during the PVM's initialization phase. No other mechanism for device interaction will be permitted.
*   The system initialization sequence will be refactored to create and populate the `mmio_db_t` *before* the first guest instruction is executed.

### **4. Success Criteria**

*   **Performance:** A dispatch call for a GPA that does *not* map to any MMIO region (the most common case) must complete in sub-microsecond time. The overhead should be dominated by the function call and a cache-hot binary search.
*   **Correctness:** 100% of MMIO accesses must be routed to the correct handler. 100% of non-MMIO accesses must be correctly identified as such (`ENOT_HANDLED`). There is no tolerance for mis-routing.
*   **Safety:** The `mmio_db_register` function must be provably correct. It must be impossible to register two overlapping MMIO regions. An attempt to do so will result in a fatal assertion failure during initialization.
*   **Test Coverage:** The MMIO dispatcher and registration logic will have 100% unit test coverage for all code paths, including error conditions and edge cases.

### **5. Proposed Design**

The design is based on the principle of **early failure and fast paths**. We do the expensive, complex work of validation and organization once at startup, allowing the runtime dispatch path to be brutally simple and fast. The core of the system is the `mmio_db_t` structure, a purpose-built database optimized for one single operation: finding the handler for a given GPA.

1.  **Data-Oriented Structure:** The `mmio_db_t` will use a Structure of Arrays (SoA) layout. The `address_ranges` and `handlers` will be stored in separate, parallel `std::vector`s. When the dispatcher searches for an address, it will only scan through the `address_ranges` vector. This maximizes CPU cache-line utilization, as the search algorithm does not need to load and pollute the cache with handler function pointers, which are irrelevant until a match is found.

2.  **Immutable Database at Runtime:** The MMIO database is configured once during PVM initialization. After the first guest instruction is executed, the database is considered immutable. There will be no mechanism for runtime registration or de-registration of MMIO regions. This simplifies the design enormously by eliminating the need for complex locking or thread-safety mechanisms on the database itself. The dispatcher can read from it without any synchronization primitives.

3.  **High-Performance Dispatch:** The `mmio_db_dispatch_*` functions will use a binary search (`std::upper_bound`) on the `address_ranges` vector, which **must** be kept sorted by `gpa_base`. This provides logarithmic time complexity, `O(log N)`, where N is the number of MMIO regions. For the expected small number of regions (`MMIO_REGIONS` is 20), this is practically constant time and is the optimal search strategy. The search logic described in the header file's comments is sound and will be the basis of the implementation.

4.  **Strict and Unforgiving Registration:** The `mmio_db_register` function is the sole gatekeeper for the database. It is not designed for performance; it is designed for absolute correctness. Before inserting a new region, it will perform a linear scan of all existing regions to guarantee that the new region does not overlap with any of them. If an overlap is detected, it will return `EADDRESS_OVERLAP`. The caller (the PVM initializer) will treat this error as a fatal configuration flaw and will `PVM_ASSERT_MSG(false, ...)` to terminate immediately. Configuration errors must be caught at boot, not during guest execution.

### **6. Technical Design**

The implementation will be based on the provided header file, with the following clarifications and mandates.

*   **`mmio_db_t`**: The use of a custom arena allocator is approved. This ensures that all MMIO metadata resides in a single, contiguous block of memory, further improving locality of reference.
*   **`mmio_db_register`**:
    1.  The function will first iterate through the *entire* existing `address_ranges` vector. For each `existing_range` in the vector, it will check if `new_range.gpa_base < existing_range.gpa_end && new_range.gpa_end > existing_range.gpa_base`. If this condition is ever true, an overlap exists. Return `EADDRESS_OVERLAP` immediately.
    2.  If no overlap is found, the function will find the correct insertion point to maintain the sort order of `address_ranges`. `std::upper_bound` can be used to find this position efficiently.
    3.  The new `range` and `handler` will be inserted into their respective vectors at the calculated position.
*   **`mmio_db_dispatch_read`/`write`**:
    1.  Use `std::upper_bound` on `db->address_ranges` with a custom comparator that only compares the search GPA with the `gpa_base` of the `mmio_range_t` elements.
    2.  This returns an iterator to the first element whose `gpa_base` is *greater than* the target GPA.
    3.  If the iterator is the beginning of the vector, no preceding range exists, so it's a non-match. Return `ENOT_HANDLED`.
    4.  Otherwise, decrement the iterator to get the candidate range (the one immediately before the one found by `upper_bound`).
    5.  Perform the final check: `gpa >= candidate.gpa_base && gpa < candidate.gpa_end`.
    6.  If it matches, retrieve the handler from the *parallel* `handlers` vector using the same index. Check if the required handler (`read` or `write`) is non-NULL. If it is NULL, return `EACCESS_DENIED`. Otherwise, call the handler and return its result.
    7.  If the final check fails, the GPA is in a hole between regions. Return `ENOT_HANDLED`.

### **7. Testing**

The correctness of this subsystem is not a matter of luck; it will be proven. A dedicated unit test suite (`test_mmio.cpp`) will be created. It will have no dependencies on the rest of the PVM.

*   **Test Harness:** A mock `kvm_t` object will be created. A set of dummy handler functions will be implemented that, when called, simply set a flag and return a specific value, allowing the test to verify that the correct handler was indeed called.

*   **Registration Tests:**
    1.  **Test Empty DB:** Test registration into a fresh, empty database.
    2.  **Test Non-Overlapping:** Register several disjoint, non-adjacent regions (e.g., `[0x1000, 0x2000)`, `[0x5000, 0x6000)`) and assert that registration succeeds and the database remains sorted.
    3.  **Test Adjacent:** Register two perfectly adjacent regions (e.g., `[0x1000, 0x2000)`, `[0x2000, 0x3000)`). This must succeed.
    4.  **Test Overlap (Fatal):** Test every possible overlap condition and assert that `mmio_db_register` returns `EADDRESS_OVERLAP`:
        *   New region completely contains an existing one.
        *   Existing region completely contains the new one.
        *   New region overlaps the end of an existing one.
        *   New region overlaps the beginning of an existing one.
        *   New region is identical to an existing one.
    5.  **Test Full DB:** If we define a hard limit like `MMIO_REGIONS`, test that attempting to register more than this limit fails correctly (this may require an assertion or a specific error code).

*   **Dispatch Tests:**
    1.  **Setup:** Create a pre-populated database with several known regions (e.g., R1:[0x1000-0x1FFF], R2:[0x4000-0x4FFF], R3:[0x8000-0x8FFF]). R1 is read-only, R2 is write-only, R3 is read-write.
    2.  **Test Hits:**
        *   Dispatch a read to the beginning, middle, and last byte of R1. Verify the R1 read handler is called.
        *   Dispatch a write to the beginning, middle, and last byte of R2. Verify the R2 write handler is called.
        *   Dispatch both reads and writes to R3 and verify correct handler invocation.
    3.  **Test Misses (Holes):**
        *   Dispatch reads and writes to addresses *between* regions (e.g., `0x3000`, `0x6000`). Assert that the return code is `ENOT_HANDLED` and no handler is called.
        *   Dispatch to an address below all regions (e.g., `0x0`).
        *   Dispatch to an address above all regions (e.g., `0x9000`).
    4.  **Test Access Denied:**
        *   Dispatch a *write* to the read-only region R1. Assert the return is `EACCESS_DENIED`.
        *   Dispatch a *read* to the write-only region R2. Assert the return is `EACCESS_DENIED`.

These tests are not optional. They are the only way to gain confidence in this critical component. They will be integrated into the CI/CD pipeline and must pass before any change to this subsystem is even considered for review. subsystem is even considered for review.
