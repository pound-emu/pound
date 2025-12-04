#ifndef POUND_HOST_MEMORY_ARENA_H
#define POUND_HOST_MEMORY_ARENA_H

#include <stddef.h>

#define POISON_PATTERN 0xAA

/*
 *  NAME
 *      pvm_host_memory_arena_t - memory management structure for efficient allocation and de-allocation.
 *
 *  SYNOPSIS
 *      typedef struct {
 *          size_t capacity;   Total number of bytes allocated.
 *          size_t size;       The current number of bytes consumed.
 *          void* data;          A pointer to the base address of the allocated memory buffer.
 *      } pvm_host_memory_arena_t;
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
    size_t capacity;
    size_t size;
    void* data;
} pvm_host_memory_arena_t;

/*
 *  NAME
 *      pvm_host_memory_arena_init - Initialize a memory arena with specified capacity.
 *
 *  SYNOPSIS
 *      pvm_host_memory_arena_t pvm_host_memory_arena_init(size_t capacity);
 *
 *  DESCRIPTION
 *     The function creates and returns a new memory arena instance with the
 *     specified capacity.
 *
 *  PARAMETERS
 *      capacity - Size of the memory arena to allocate in bytes
 *
 *  RETURN VALUE
 *     Returns a valid pvm_host_memory_arena_t object on success. pvm_host_memory_arena_t->data will be null on failure.
 */
pvm_host_memory_arena_t pvm_host_memory_arena_init(size_t capacity);
/*
 *  NAME
 *      pvm_host_memory_arena_allocate - Allocate memory from a pre-initialized arena.
 *
 *  SYNOPSIS
 *      void* pvm_host_memory_arena_allocate(pvm_host_memory_arena_t* arena, size_t size);
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
 *      Requires pvm_host_memory_arena_t to be initialized with pvm_host_memory_arena_init() or similar.
 */
void* pvm_host_memory_arena_allocate(pvm_host_memory_arena_t* arena, const size_t size);

/*
 *  NAME
 *      pvm_host_memory_arena_reset - Reset a memory arena's allocation size to zero.
 *
 *  SYNOPSIS
 *      void pvm_host_memory_arena_reset(pvm_host_memory_arena_t* arena);
 *
 *  DESCRIPTION
 *      The function resets the allocation size of a pre-initialized pvm_host_memory_arena_t to zero.
 *      This effectively "frees" all memory allocated from the arena without
 *      deallocating the underlying buffer, allowing reuse of the capacity for
 *      future allocations.
 *
 *  NOTES
 *      Resets arena->size to 0 while preserving arena->capacity.
 *      Does not free the underlying memory buffer.
 *      Useful for reusing arenas without reallocation.
 */
void pvm_host_memory_arena_reset(pvm_host_memory_arena_t* arena);

/**
 *  NAME
 *      pvm_host_memory_arena_free - Free the memory allocated by an arena
 *
 *  SYNOPSIS
 *      void pvm_host_memory_arena_free(pvm_host_memory_arena_t* arena);
 *
 *  DESCRIPTION
 *      The function releases the memory buffer associated with a pvm_host_memory_arena_t and
 *      resets its capacity and size to zero. This marks the arena as invalid for
 *      future allocation unless reinitialized.
 */
void pvm_host_memory_arena_free(pvm_host_memory_arena_t* arena);
#endif  //POUND_HOST_MEMORY_ARENA_H
