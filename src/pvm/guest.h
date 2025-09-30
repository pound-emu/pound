#ifndef POUND_pvm_GUEST_H
#define POUND_pvm_GUEST_H

#include <cassert>
#include <stdint.h>
#include <string.h>

#include "endian.h"

#include "host/memory/arena.h"

namespace pound::pvm::memory
{

/*
 * guest_memory_t - A non-owning descriptor for a block of guest physical RAM.
 * @base: Pointer to the start of the host-allocated memory block.
 * @size: The size of the memory block in bytes.
 *
 *
 * This structure describes a contiguous block of guest physical memory. It acts
 * as a handle or a "view" into a region of host memory, but it does not manage
 * the lifetime of that memory itself.
 *
 * --- Ownership ---
 * The guest_memory_t struct does NOT own the memory block pointed to by @base.
 * Ownership of the underlying memory buffer is retained by the host memory
 * arena from which it was allocated. The party responsible for creating the
 * arena is also responsible for ultimately freeing it. This struct is merely a
 * descriptor and can be safely passed by value or pointer without transferring
 * ownership.
 *
 * --- Lifetime ---
 * An instance of this struct should be considered valid only for as long as the
 * backing memory arena is valid. Typically, this means it is created once
- * during virtual machine initialization and lives for the entire duration of
 * the emulation session. Its lifetime is tied to the lifetime of the parent
 * pvm instance.
 *
 * --- Invariants ---
 * Both fields of this struct are declared `const`. This establishes the 
 * invariant that once a guest_memory_t descriptor is created and initialized
 * by guest_memory_create(), its size and base address are immutable for the
 * lifetime of the object. This prevents accidental resizing or repointing of
 * the guest's physical RAM.
 */
typedef struct
{
    uint8_t* const base;
    const uint64_t size;
} guest_memory_t;

/*
 * guest_memory_create() - Allocates and initializes a guest memory region from
 * an arena.
 * @arena: A pointer to a host memory arena that will be the source for all 
 *         allocations.
 *
 * This function sets up the primary guest RAM block. It uses a provided host 
 * memory arena as the backing store for both the guest_memory_t descriptor
 * struct and the guest RAM itself.
 *
 * The function first allocates a small chunk from the arena for the guest_memory_t
 * struct. It then dedicates the *entire remaining capacity* of the arena to be
 * the main guest RAM block.
 *
 * Preconditions:
 *  - @arena must be a valid, non-NULL pointer to an initialized host arena.
 *  - @arena->data must point to a valid, host-allocated memory buffer.
 *  - The arena provided should be dedicated solely to this guest memory block;
 *    its entire remaining capacity will be consumed.
 *
 * Return: A pointer to a fully initialized guest_memory_t struct. The `base`
 * pointer will point to the start of the guest RAM block within the arena,
 * and `size` will reflect the size of that block.
 */
guest_memory_t* guest_memory_create(pound::host::memory::arena_t* arena);

/*
 * guest_mem_access_result_t - Defines the set of possible outcomes for a guest
 * memory access operation.
 * @GUEST_MEM_ACCESS_OK:                The memory operation completed
 *                                      successfully.
 * @GUEST_MEM_FAULT_UNALIGNED:          The access was unaligned, and the 
 *                                      emulated CPU requires an Alignment
 *                                      Fault to be raised. The operation was 
 *                                      NOT completed. The host must inject a
 *                                      data abort into the guest.
 * @GUEST_MEM_ACCESS_FAULT_BOUNDARY:    An access fell outside the bounds of
 *                                      the defined memory region. The
 *                                      operation was NOT completed, The host
 *                                      must inject a Data Abort for a
 *                                      translation/permission fault into the
 *                                      guest.
 * @GUEST_MEM_ACCESS_ERROR_INTERNAL:    An unrecoverable internal error occured
 *                                      within the memory subsystem. This
 *                                      indicates a fatal host bug, not a guest
 *                                      induced fault.
 */
typedef enum
{
    GUEST_MEM_ACCESS_OK = 0,
    GUEST_MEM_ACCESS_FAULT_UNALIGNED,
    GUEST_MEM_ACCESS_FAULT_BOUNDARY,
    GUEST_MEM_ACCESS_ERROR_INTERNAL,
} guest_mem_access_result_t;

/*
 * ============================================================================
 *                          Guest Memory Read Functions
 * ============================================================================
 */

/*
 * guest_mem_readb() - Read one byte from guest physical memory.
 * @memory:     A pointer to the guest memory region.
 * @gpa:        The guest physical address to read from.
 * @out_val:    A pointer to a uint8_t where the result will be stored.
 *
 * This function safely reads a single 8-bit value from the guest's physical
 * RAM. It performs a bounds check to ensure the access is within the allocated
 * memory region.
 *
 * Preconditions:
 *  - @memory and @out_val must be valid, non-NULL pointers.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *
 * Return: 
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY if the @gpa is outside the valid memory
 * range.
 */
inline guest_mem_access_result_t guest_mem_readb(guest_memory_t* memory, uint64_t gpa, uint8_t* out_val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(nullptr != out_val);

    if (gpa >= memory->size)
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    uint8_t* hva = memory->base + gpa;
    *out_val = *hva;

    return GUEST_MEM_ACCESS_OK;
}

/*
 * guest_mem_readw() - Read a 16-bit word from guest physical memory.
 * @memory:     A pointer to the guest memory region.
 * @gpa:        The guest physical address to read from.
 * @out_val:    A pointer to a uint16_t where the result will be stored.
 *
 * This function safely reads a 16-bit little-endian value from guest RAM.
 * It performs both boundary and alignment checks before the access.
 * It will also perform a byte swap if the host system is not little-endian.
 *
 * Preconditions:
 *  - @memory and @out_val must be valid, non-NULL pointers.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *  - The guest address @gpa must be 2-byte aligned.
 *
 * Return: 
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY on an out-of-bounds access or
 * %GUEST_MEM_ACCESS_FAULT_UNALIGNED if @gpa is not 2-byte aligned.
 */
inline guest_mem_access_result_t guest_mem_readw(guest_memory_t* memory, uint64_t gpa, uint16_t* out_val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(nullptr != out_val);

    if (gpa > (memory->size - sizeof(uint16_t)))
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    if ((gpa & 1) != 0)
    {
        return GUEST_MEM_ACCESS_FAULT_UNALIGNED;
    }

    uint8_t* hva = memory->base + gpa;
    memcpy(out_val, hva, sizeof(uint16_t));

#if HOST_IS_LITTLE_ENDIAN != GUEST_IS_LITTLE_ENDIAN
    *out_val = bswap_16(*out_val);
#endif
    return GUEST_MEM_ACCESS_OK;
}

/*
 * guest_mem_readl() - Read a 32-bit long-word from guest physical memory.
 * @memory:     A pointer to the guest memory region.
 * @gpa:        The guest physical address to read from.
 * @out_val:    A pointer to a uint32_t where the result will be stored.
 *
 * This function safely reads a 32-bit little-endian value from guest RAM.
 * It performs both boundary and alignment checks before the access.
 * It will also perform a byte swap if the host system is not little-endian.
 *
 * Preconditions:
 *  - @memory and @out_val must be valid, non-NULL pointers.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *  - The guest address @gpa must be 4-byte aligned.
 *
 * Return: 
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY on an out-of-bounds access or
 * %GUEST_MEM_ACCESS_FAULT_UNALIGNED if @gpa is not 4-byte aligned.
 */
inline guest_mem_access_result_t guest_mem_readl(guest_memory_t* memory, uint64_t gpa, uint32_t* out_val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(nullptr != out_val);

    if (gpa > (memory->size - sizeof(uint32_t)))
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    if ((gpa & 3) != 0)
    {
        return GUEST_MEM_ACCESS_FAULT_UNALIGNED;
    }

    uint8_t* hva = memory->base + gpa;
    memcpy(out_val, hva, sizeof(uint32_t));

#if HOST_IS_LITTLE_ENDIAN != GUEST_IS_LITTLE_ENDIAN
    *out_val = bswap_32(*out_val);
#endif
    return GUEST_MEM_ACCESS_OK;
}

/*
 * guest_mem_readq() - Read a 64-bit quad-word from guest physical memory.
 * @memory:     A pointer to the guest memory region.
 * @gpa:        The guest physical address to read from.
 * @out_val:    A pointer to a uint64_t where the result will be stored.
 *
 * This function safely reads a 64-bit little-endian value from guest RAM.
 * It performs both boundary and alignment checks before the access. 
 * It will also perform a byte swap if the host system is not little-endian.
 *
 * Preconditions:
 *  - @memory and @out_val must be valid, non-NULL pointers.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *  - The guest address @gpa must be 8-byte aligned.
 *
 * Return:
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY on an out-of-bounds access or
 * %GUEST_MEM_ACCESS_FAULT_UNALIGNED if @gpa is not 8-byte aligned.
 */
inline guest_mem_access_result_t guest_mem_readq(guest_memory_t* memory, uint64_t gpa, uint64_t* out_val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(nullptr != out_val);

    if (gpa > (memory->size - sizeof(uint64_t)))
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    if ((gpa & 7) != 0)
    {
        return GUEST_MEM_ACCESS_FAULT_UNALIGNED;
    }

    uint8_t* hva = memory->base + gpa;
    memcpy(out_val, hva, sizeof(uint64_t));

#if HOST_IS_LITTLE_ENDIAN != GUEST_IS_LITTLE_ENDIAN
    *out_val = bswap_64(*out_val);
#endif
    return GUEST_MEM_ACCESS_OK;
}

/*
 * ============================================================================
 *                          Guest Memory Write Functions
 * ============================================================================
 */

/*
 * guest_mem_writeb() - Write one byte to guest physical memory.
 * @memory: A pointer to the guest memory region.
 * @gpa:    The guest physical address to write to.
 * @val:    The 8-bit value to write.
 *
 * This function safely writes a single 8-bit value to the guest's physical
 * RAM. It performs a bounds check to ensure the access is within the allocated
 * memory region before performing the write.
 *
 * Preconditions:
 *  - @memory must be a valid, non-NULL pointer.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *
 * Return: 
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY if the @gpa is outside the valid memory
 * range.
 */
inline guest_mem_access_result_t guest_mem_writeb(guest_memory_t* memory, uint64_t gpa, uint8_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);

    if (gpa >= memory->size)
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    uint8_t* hva = memory->base + gpa;
    *hva = val;
    return GUEST_MEM_ACCESS_OK;
}

/*
 * guest_mem_writew() - Write a 16-bit word to guest physical memory.
 * @memory: A pointer to the guest memory region.
 * @gpa:    The guest physical address to write to.
 * @val:    The 16-bit value to write.
 *
 * This function safely writes a 16-bit little-endian value to guest RAM.
 * It performs both boundary and alignment checks before the access.
 * It will also perform a byte swap if the host system is not little-endian.
 *
 * Preconditions:
 *  - @memory must be a valid, non-NULL pointer.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *  - The guest address @gpa must be 2-byte aligned.
 *
 * Return: %GUEST_MEM_ACCESS_OK on success. Returns
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY on an out-of-bounds access or
 * %GUEST_MEM_ACCESS_FAULT_UNALIGNED if @gpa is not 2-byte aligned.
 */
inline guest_mem_access_result_t guest_mem_writew(guest_memory_t* memory, uint64_t gpa, uint16_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);

    if (gpa > (memory->size - sizeof(uint16_t)))
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    if ((gpa & 1) != 0)
    {
        return GUEST_MEM_ACCESS_FAULT_UNALIGNED;
    }

    uint8_t* hva = memory->base + gpa;

#if HOST_IS_LITTLE_ENDIAN != GUEST_IS_LITTLE_ENDIAN
    val = bswap_16(val);
#endif

    memcpy(hva, &val, sizeof(uint16_t));
    return GUEST_MEM_ACCESS_OK;
}

/*
 * guest_mem_writel() - Write a 32-bit long-word to guest physical memory.
 * @memory: A pointer to the guest memory region.
 * @gpa:    The guest physical address to write to.
 * @val:    The 32-bit value to write.
 *
 * This function safely writes a 32-bit little-endian value to guest RAM.
 * It performs both boundary and alignment checks before the access.
 * It will also perform a byte swap if the host system is not little-endian.
 *
 * Preconditions:
 *  - @memory must be a valid, non-NULL pointer.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *  - The guest address @gpa must be 4-byte aligned.
 *
 * Return: 
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY on an out-of-bounds access or
 * %GUEST_MEM_ACCESS_FAULT_UNALIGNED if @gpa is not 4-byte aligned.
 */
inline guest_mem_access_result_t guest_mem_writel(guest_memory_t* memory, uint64_t gpa, uint32_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);

    if (gpa > (memory->size - sizeof(uint32_t)))
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    if ((gpa & 3) != 0)
    {
        return GUEST_MEM_ACCESS_FAULT_UNALIGNED;
    }

    uint8_t* hva = memory->base + gpa;

#if HOST_IS_LITTLE_ENDIAN != GUEST_IS_LITTLE_ENDIAN
    val = bswap_32(val);
#endif

    memcpy(hva, &val, sizeof(uint32_t));
    return GUEST_MEM_ACCESS_OK;
}

/*
 * guest_mem_writeq() - Write a 64-bit quad-word to guest physical memory.
 * @memory: A pointer to the guest memory region.
 * @gpa:    The guest physical address to write to.
 * @val:    The 64-bit value to write.
 *
 * This function safely writes a 64-bit little-endian value to guest RAM.
 * It performs both boundary and alignment checks before the access.
 * It will also perform a byte swap if the host system is not little-endian.
 *
 * Preconditions:
 *  - @memory must be a valid, non-NULL pointer.
 *  - @memory->base must point to a valid, host-allocated memory buffer.
 *  - The guest address @gpa must be 8-byte aligned.
 *
 * Return: 
 * %GUEST_MEM_ACCESS_OK on success.
 * %GUEST_MEM_ACCESS_FAULT_BOUNDARY on an out-of-bounds access or
 * %GUEST_MEM_ACCESS_FAULT_UNALIGNED if @gpa is not 8-byte aligned.
 */
inline guest_mem_access_result_t guest_mem_writeq(guest_memory_t* memory, uint64_t gpa, uint64_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);

    if (gpa > (memory->size - sizeof(uint64_t)))
    {
        return GUEST_MEM_ACCESS_FAULT_BOUNDARY;
    }

    if ((gpa & 7) != 0)
    {
        return GUEST_MEM_ACCESS_FAULT_UNALIGNED;
    }

    uint8_t* hva = memory->base + gpa;

#if HOST_IS_LITTLE_ENDIAN != GUEST_IS_LITTLE_ENDIAN
    val = bswap_64(val);
#endif

    memcpy(hva, &val, sizeof(uint64_t));
    return GUEST_MEM_ACCESS_OK;
}
}  // namespace pound::pvm::memory
#endif  // POUND_pvm_GUEST_H
