#pragma once

#include <cstdint>
#include <vector>
#include "host/memory/arena_stl.h"
#include "kvm.h"

namespace pound::kvm::memory
{
/*
 * MMIO_REGIONS - The maximum number of distinct MMIO regions supported.
 *
 * It sets a hard limit on how many separate hardware device regions
 * can be registered at boot time.
 */
#define MMIO_REGIONS 20

/* MMIO_SUCCESS - Return code for a successfull MMIO operation. */
#define MMIO_SUCCESS 0

/* EADDRESS_OVERLAP - Error code for an MMIO address space conflict. */
#define EADDRESS_OVERLAP (-1)

#define ENOT_HANDLED (-2)

#define EACCESS_DENIED (-3)

/*
 * typedef mmio - Function pointer type for an MMIO access handler.
 * @kvm:    A pointer to the KVM instance.
 * @gpa:    The guest physical address of the access.
 * @data:   A pointer to the data buffer. For reads, this buffer 
 *          should be filled by the handler. For writes, this buffer
 *          contains the data written by the guest.
 * @len:    The size of the access in bytes.
 *
 * This function pointer defines the contract for all MMIO read
 * and write handler functions. Handlers are responsible for emulating
 * the hardware's response to a memory access at a specific register
 * address.
 *
 * Returns: MMIO_SUCCESS on success, negative errno code on failure.
 */
typedef int8_t (*mmio)(kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len);

/*
 * mmio_handler_t - A pair of handlers for an MMIO region.
 * @read:   A function pointer to be called for read access within the
 *          region. Can be NULL if the region is write-only.
 * @write:  A function pointer to be called for write access within the
 *          region. Can NULL if the region is read-only.
 *
 * This structure stores the read and write operations for a single
 * hardware device or memory region.
 *
 */
typedef struct
{
    mmio read;
    mmio write;
} mmio_handler_t;

/*
 * mmio_range_t - Defines a half-open guest physical address range.
 * @gpa_base:   The starting (inclusive) guest physical address of
 *              the region.
 * @gpa_end:    The ending (exclusive) guest physical address of the region.
 *
 * This structure defines a contiguous block of guest physical address
 * space, [gpa_base, gpa_end). The use of an exclusive end address
 * simplifies range and adjacency calculations.
 */
typedef struct
{
    uint64_t gpa_base;
    uint64_t gpa_end;
} mmio_range_t;

/*
 * mmio_db_t - A data-oriented database for MMIO dispatch.
 * @handlers:       A vector of MMIO handler pairs.
 * @address_ranges: A vector of physical address ranges, sorted by GPA base.
 *
 * This structure manages all registered Memory-Mapped I/O regions for a
 * virtual machine. It is designed with a "Structure of Arrays" layout
 * to maximize host CPU cache efficiency during lookups.
 */
typedef struct
{
    /*
     * This uses a custom arena allocator to ensure that all handler nodes
     * are allocated fron a single, pre-allocated memory block.
     *
     * This is a parallel array to @address_ranges.
     */
    std::vector<mmio_handler_t, pound::host::memory::arena_allocator<mmio_handler_t>> handlers;

    /*
     * This vector is the primary target for the binary search lookup
     * in the MMIO dispatcher. Maintaining its sort order is critical
     * for the performance of the system.
     */
    std::vector<mmio_range_t, pound::host::memory::arena_allocator<mmio_range_t>> address_ranges;
} mmio_db_t;

/*
 * Registers a new MMIO region into the database.
 * @db:         A pointer to the MMIO database to be modified.
 * @range:      The new region's address space.
 * @handler:    The read and write callbacks.
 *
 * This function safely inserts a new MMIO region into the database.
 *
 * Returns: 
 * MMIO_SUCCESS on successfull registration.
 * EADDRESS_OVERLAP if the new @range conflicts with any existing region.
 */
int8_t mmio_db_register(mmio_db_t* db, const mmio_range_t range, const mmio_handler_t handler);

/*
 * mmio_db_dispatch_write - Dispatches a guest physical write to a registered MMIO handler.
 * @db:    A pointer to the MMIO database to be queried.
 * @kvm:   A pointer to the KVM instance.
 * @gpa:   The guest physical address of the memory write.
 * @data:  A pointer to the buffer containing the data written by the guest.
 * @len:   The size of the write access in bytes.
 *
 * This function is on the critical path ("hot path") of the emulator. It
 * performs a high-performance binary search to determine if the target @gpa
 * falls within any of the registered MMIO regions.
 *
 * The logic is a two-stage process:
 * 1. An approximate search using std::upper_bound finds the first region that
 *    starts *after* the target @gpa. The actual candidate region must be the
 *    one immediately preceding this result.
 * 2. A precise check verifies if the @gpa is contained within the candidate
 *    region's half-open interval [base, end).
 *
 * If a match is found, the corresponding write handler is invoked. If not, the
 * function signals that the access is not handled by the MMIO system and
 * should be treated as a normal RAM access.
 *
 * --- Visual Scenario ---
 *
 * Database Ranges:  [-- R1 --)     [---- R2 ----)      [--- R3 ---)
 * Address Space:    0x1000 0x1010   0x4000     0x4080   0x9000  0x9010
 *
 * Search for GPA = 0x4020:
 *
 * 1. upper_bound() finds the first region starting > 0x4020, which is R3.
 *    The iterator 'it' points to R3 at index 2.
 *
 *          [-- R1 --)     [---- R2 ----)      [--- R3 ---)
 *                                               ^
 *                                               |
 *                                              'it'
 *
 * 2. The candidate for the search is the region before 'it', which is R2.
 *
 *          [-- R1 --)     [---- R2 ----)      [--- R3 ---)
 *                                ^
 *                                |
 *                             candidate
 *
 * 3. Final check: Is 0x4020 >= R2.base (0x4000) AND < R2.end (0x4080)? Yes.
 *    Result: Match found. Dispatch to handler for R2.
 *
 * Return: 
 * MMIO_SUCCESS if the write was handled by a registered device. Returns
 * ENOT_HANDLED if the @gpa does not map to any MMIO region.
 * EACCESS_DENIED if the MMIO region has no write function pointer.
 */
int8_t mmio_db_dispatch_write(mmio_db_t* db, kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len);

/*
 * mmio_db_dispatch_read - Dispatches a guest physical read to a registered MMIO handler.
 * @db:    A pointer to the MMIO database to be queried.
 * @kvm:   A pointer to the KVM instance.
 * @gpa:   The guest physical address of the memory write.
 * @data:  A pointer to the buffer containing the data written by the guest.
 * @len:   The size of the write access in bytes.
 *
 * See @mmio_db_dispatch_write() for proper explanation.
 *
 * Return: 
 * MMIO_SUCCESS if the write was handled by a registered device. Returns
 * ENOT_HANDLED if the @gpa does not map to any MMIO region.
 * EACCESS_DENIED if the MMIO region has no write function pointer.
 */
int8_t mmio_db_dispatch_read(mmio_db_t* db, kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len);
}  // namespace pound::kvm::memory
