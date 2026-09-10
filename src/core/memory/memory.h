#ifndef POUND_MEMORY_H
#define POUND_MEMORY_H

typedef enum
{
    MEMORY_ALLOCATOR_MIMALLOC,
} memory_allocator_type_t;

typedef enum
{
    MEMORY_BUCKET_UI,
    MEMORY_BUCKET_GUEST_MEMORY,
    MEMORY_BUCKET_JIT_RECOMPILER,
    MEMORY_BUCKET_DEBUG_PROFILING,
    MEMORY_BUCKET_COUNT,
} memory_bucket_type_t;

/// Controls all of Pound's memory.
typedef struct
{
    // TODO
} memory_subsystem_t;

#endif // POUND_MEMORY_H

/*** end of file ***/