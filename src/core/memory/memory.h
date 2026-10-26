#ifndef POUND_MEMORY_H
#define POUND_MEMORY_H

#include "attributes.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_PAGE_SHIFT 12ULL
#define MEMORY_PAGE_SIZE  (1ULL << MEMORY_PAGE_SHIFT)

typedef enum
{
    MEMORY_HEAP_TYPE_HOST,
    MEMORY_HEAP_TYPE_JIT_EXECUTABLE,
    MEMORY_HEAP_TYPE_DEBUG_PROFILING,
    MEMORY_HEAP_TYPE_COUNT,
} memory_heap_type_t;

typedef enum
{
    MEMORY_BUCKET_NONE = 0,
    MEMORY_BUCKET_UI,
    MEMORY_BUCKET_GUEST_MEMORY,
    MEMORY_BUCKET_JIT_RECOMPILER,
    MEMORY_BUCKET_DEBUG_PROFILING,
    MEMORY_BUCKET_COUNT,
} memory_bucket_type_t;

POUND_ALIGNED(64) typedef struct
{
    uint64_t used_bytes;
    uint64_t page_count;
    uint64_t allocation_count;
    char     pad[40];
} memory_bucket_stats_t;

static_assert(64 == sizeof(memory_bucket_stats_t), "Struct size mismatch");

typedef struct memory_allocator memory_allocator_t;

POUND_CORE_API void                   memory_subsystem_init(void);
POUND_CORE_API void                   memory_subsystem_destroy(void);
POUND_CORE_API memory_allocator_t    *memory_subsystem_set_allocator(memory_allocator_t *POUND_RESTRICT allocator);
POUND_CORE_API memory_allocator_t    *memory_subsystem_get_allocator(void);
POUND_CORE_API void                  *memory_subsystem_allocate(size_t alignment, size_t bytes);
POUND_CORE_API void                   memory_subsystem_free(void *POUND_RESTRICT pointer);
POUND_CORE_API memory_bucket_type_t   memory_subsystem_set_bucket(memory_bucket_type_t bucket);
POUND_CORE_API memory_bucket_stats_t *memory_subsystem_get_bucket_stats(memory_bucket_type_t bucket);
POUND_CORE_API size_t                 memory_subsystem_get_usable_size(const void *POUND_RESTRICT pointer);
POUND_CORE_API size_t                 memory_subsystem_get_bytes_used_by_bucket(int bucket);
POUND_CORE_API size_t                 memory_subsystem_get_heap_size(void);

extern POUND_CORE_API memory_allocator_t g_host_allocator;

#endif // POUND_MEMORY_H

/*** end of file ***/
