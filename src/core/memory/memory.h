#ifndef POUND_MEMORY_H
#define POUND_MEMORY_H

#include "attributes.h"
#include <stddef.h>

typedef enum
{
    MEMORY_HEAP_TYPE_HOST,
    MEMORY_HEAP_TYPE_JIT_EXECUTABLE,
    MEMORY_HEAP_TYPE_COUNT,
} memory_heap_type_t;

typedef enum
{
    MEMORY_BUCKET_UI,
    MEMORY_BUCKET_GUEST_MEMORY,
    MEMORY_BUCKET_JIT_RECOMPILER,
    MEMORY_BUCKET_DEBUG_PROFILING,
    MEMORY_BUCKET_COUNT,
} memory_bucket_type_t;

typedef struct memory_allocator memory_allocator_t;

struct memory_allocator
{
    void *(*allocate)(memory_allocator_t *POUND_RESTRICT allocator, size_t alignment, size_t size);
    void (*free)(memory_allocator_t *POUND_RESTRICT allocator, void *pointer);
};

/// Controls all of Pound's memory.
typedef struct
{
} memory_subsystem_t;

void                memory_subsystem_init(void);
void                memory_subsystem_destroy(void);
memory_allocator_t *memory_subsystem_set_allocator(memory_allocator_t *POUND_RESTRICT allocator);

extern memory_allocator_t g_host_allocator;

#endif // POUND_MEMORY_H

/*** end of file ***/