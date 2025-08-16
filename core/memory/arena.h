#ifndef POUND_ARENA_H
#define POUND_ARENA_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace pound::memory
{
#define POISON_PATTERN 0xAA

/*
 *  NAME
 *      arena_t - memory management structure for efficient allocation and de-allocation.
 *
 *  SYNOPSIS
 *      typedef struct {
 *          std::size_t capacity;   Total number of bytes allocated.
 *          std::size_t size;       The current number of bytes consumed.
 *          void* data;          A pointer to the base address of the allocated memory buffer.
 *      } arena_t;
 *
 *  DESCRIPTION
 *      The arena struct handles allocating and managing contiguous memory blocks.
 *
 *  RATIONALE
 *      A memory arena offers a safer alternative to malloc/realloc by
 *      maintaining a single contiguous block eliminates heap fragmentation
 *      that occurs with frequent small allocations.
 */
typedef struct
{
    std::size_t capacity;
    std::size_t size;
    void* data;
} arena_t;

/*
 *  NAME
 *      arena_init - Initialize a memory arena with specified capacity.
 *
 *  SYNOPSIS
 *      memory::arena_t memory::arena_init(size_t capacity);
 *
 *  DESCRIPTION
 *     The function creates and returns a new memory arena instance with the
 *     specified capacity.
 *
 *  PARAMETERS
 *      capacity - Size of the memory arena to allocate in bytes
 *
 *  RETURN VALUE
 *     Returns a valid memory::arena_t object on success. arena_t->data will be null on failure.
 */
memory::arena_t arena_init(size_t capacity);
/*
 *  NAME
 *      arena_allocate - Allocate memory from a pre-initialized arena.
 *
 *  SYNOPSIS
 *      const void* memory::arena_allocate(memory::arena_t* arena, std::size_t size);
 *
 *  DESCRIPTION
 *      The function allocates size bytes from the specified arena. It assumes
 *      the arena has sufficient capacity and returns a pointer to the allocated
 *      memory region.
 *
 *  RETURN VALUE
 *      Returns a pointer to the first byte of the allocated memory. The returned
 *      pointer is valid until the arena is reset or destroyed.
 *
 *  NOTES
 *      Requires arena_t to be initialized with arena_init() or similar.
 */
const void* arena_allocate(arena_t* arena, const std::size_t size);

/*
 *  NAME
 *      arena_reset - Reset a memory arena's allocation size to zero.
 *
 *  SYNOPSIS
 *      void memory::arena_reset(memory::arena_t* arena);
 *
 *  DESCRIPTION
 *      The function resets the allocation size of a pre-initialized arena_t to zero.
 *      This effectively "frees" all memory allocated from the arena without
 *      deallocating the underlying buffer, allowing reuse of the capacity for
 *      future allocations.
 *
 *  NOTES
 *      Resets arena->size to 0 while preserving arena->capacity.
 *      Does not free the underlying memory buffer.
 *      Useful for reusing arenas without reallocation.
 */
void arena_reset(arena_t* arena);

/**
 *  NAME
 *      arena_free - Free the memory allocated by an arena
 *
 *  SYNOPSIS
 *      void memory::arena_free(memory::arena_t* arena);
 *
 *  DESCRIPTION
 *      The function releases the memory buffer associated with a arena_t and
 *      resets its capacity and size to zero. This marks the arena as invalid for
 *      future allocation unless reinitialized.
 */
void arena_free(memory::arena_t* arena);

}  // namespace pound::memory
#endif  //POUND_ARENA_H
