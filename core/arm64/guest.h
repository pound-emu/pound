#pragma once

#include <cassert>

namespace pound::arm64::memory
{

/*
 * guest_memory_t - Describes a contiguous block of guest physical RAM.
 * @base: Pointer to the start of the host-allocated memory block.
 * @size: The size of the memory block in bytes.
 */
typedef struct
{
    uint8_t* base;
    uint64_t size;
} guest_memory_t;

/*
 * gpa_to_hva() - Translate a Guest Physical Address to a Host Virtual Address.
 * @memory: The guest memory region to translate within.
 * @gpa: The Guest Physical Address (offset) to translate.
 *
 * This function provides a fast, direct translation for a flat guest memory
 * model. It relies on the critical pre-condition that the guest's physical
 * RAM is backed by a single, contiguous block of virtual memory in the host's
 * userspace (typically allocated with mmap()).
 *
 * In this model, memory->base is the Host Virtual Address (HVA) of the start of
 * the backing host memory. The provided Guest Physical Address (gpa) is not
 * treated as a pointer, but as a simple byte offset from the start of the guest's
 * physical address space (PAS).
 *
 * The translation is therefore a single pointer-offset calculation. This establishes
 * a direct 1:1 mapping between the guest's PAS and the host's virtual memory block.
 *
 * The function asserts that GPA is within bounds. The caller is responsible for
 * ensuring the validity of the GPA prior to calling.
 *
 * Return: A valid host virtual address pointer corresponding to the GPA.
 */
static inline uint8_t* gpa_to_hva(guest_memory_t* memory, uint64_t gpa)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(gpa < memory->size);
    uint8_t* hva = memory->base + gpa;
    return hva;
}

// TODO(GloriousTacoo:aarch64) Implement big to little endian conversion for guest_mem read and write functions.

/*
 * ============================================================================
 *                          Guest Memory Read Functions
 * ============================================================================
 */

/*
 * guest_mem_readb() - Read one byte from guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to read from.
 * Returns the 8-bit value read from memory.
 */
static inline uint8_t guest_mem_readb(guest_memory_t* memory, uint64_t gpa)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(gpa <= memory->size);
    uint8_t* hva = gpa_to_hva(memory, gpa);
    return *hva;
}

/*
 * guest_mem_readw() - Read a 16-bit word from guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to read from (must be 2-byte aligned).
 * Returns the 16-bit value, corrected for host endianness.
 */
static inline uint16_t guest_mem_readw(guest_memory_t* memory, uint64_t gpa)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert((gpa + sizeof(uint16_t)) <= memory->size);
    // Check if gpa is aligned to 2 bytes.
    assert((gpa & 1) == 0);
    uint16_t* hva = (uint16_t*)gpa_to_hva(memory, gpa);
    return *hva;
}

/*
 * guest_mem_readl() - Read a 32-bit long-word from guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to read from (must be 4-byte aligned).
 * Returns the 32-bit value, corrected for host endianness.
 */
static inline uint32_t guest_mem_readl(guest_memory_t* memory, uint64_t gpa)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert((gpa + sizeof(uint32_t)) <= memory->size);
    // Check if gpa is aligned to 4 bytes.
    assert((gpa & 3) == 0);
    uint32_t* hva = (uint32_t*)gpa_to_hva(memory, gpa);
    return *hva;
}

/*
 * guest_mem_readq() - Read a 64-bit quad-word from guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to read from (must be 8-byte aligned).
 * Returns the 64-bit value, corrected for host endianness.
 */
static inline uint64_t guest_mem_readq(guest_memory_t* memory, uint64_t gpa)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert((gpa + sizeof(uint64_t)) <= memory->size);
    // Check if gpa is aligned to 8 bytes.
    assert((gpa & 7) == 0);
    uint64_t* hva = (uint64_t*)gpa_to_hva(memory, gpa);
    return *hva;
}

/*
 * ============================================================================
 *                          Guest Memory Write Functions
 * ============================================================================
 */

/*
 * guest_mem_writeb() - Write one byte to guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to write to.
 * @val:    The 8-bit value to write.
 */
static inline void guest_mem_writeb(guest_memory_t* memory, uint64_t gpa, uint8_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert(gpa <= memory->size);
    uint8_t* hva = gpa_to_hva(memory, gpa);
    *hva = val;
}

/*
 * guest_mem_writew() - Write a 16-bit word to guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to write to (must be 2-byte aligned).
 * @val:    The 16-bit value to write (will be converted to guest endianness).
 */
static inline void guest_mem_writew(guest_memory_t* memory, uint64_t gpa, uint16_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert((gpa + sizeof(uint16_t)) <= memory->size);
    // Check if gpa is aligned to 2 bytes.
    assert((gpa & 1) == 0);
    uint16_t* hva = (uint16_t*)gpa_to_hva(memory, gpa);
    *hva = val;
}

/*
 * guest_mem_writel() - Write a 32-bit long-word to guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to write to (must be 4-byte aligned).
 * @val:    The 32-bit value to write.
 */
static inline void guest_mem_writel(guest_memory_t* memory, uint64_t gpa, uint32_t val)
{
    assert(nullptr != memory->base);
    assert((gpa + sizeof(uint32_t)) <= memory->size);
    // Check if gpa is aligned to 4 bytes.
    assert((gpa & 3) == 0);
    uint32_t* hva = (uint32_t*)gpa_to_hva(memory, gpa);
    *hva = val;
}

/*
 * guest_mem_writeq() - Write a 64-bit quad-word to guest memory.
 * @memory: The guest memory region.
 * @gpa:    The Guest Physical Address to write to (must be 8-byte aligned).
 * @val:    The 64-bit value to write.
 */
static inline void guest_mem_writeq(guest_memory_t* memory, uint64_t gpa, uint64_t val)
{
    assert(nullptr != memory);
    assert(nullptr != memory->base);
    assert((gpa + sizeof(uint64_t)) <= memory->size);
    // Check if gpa is aligned to 8 bytes.
    assert((gpa & 7) == 0);
    uint64_t* hva = (uint64_t*)gpa_to_hva(memory, gpa);
    *hva = val;
}
}  // namespace pound::aarch64::memory
