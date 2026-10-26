#include "memory.h"
#include "log.h"
#include "platform.h"

#include "mimalloc-stats.h"
#include "mimalloc.h"
#include <stdlib.h>

#if POUND_PLATFORM_WINDOWS
// mi_malloc_usable_size symbol it's not found on windows build so we remap to the mi_usable_size
// function
#define mi_malloc_usable_size mi_usable_size
#endif

struct memory_allocator
{
    memory_bucket_stats_t bucket_stats[MEMORY_BUCKET_COUNT];
    size_t                total_bytes_used;
    char                  pad[24];

    void *(*allocate)(memory_allocator_t *POUND_RESTRICT allocator, size_t alignment, size_t bytes);
    void (*free)(memory_allocator_t *POUND_RESTRICT allocator, void *pointer);
    size_t (*get_usable_size)(const void *POUND_RESTRICT pointer);
    size_t (*get_heap_size)(void);
};

static void  *host_allocate(memory_allocator_t *POUND_RESTRICT allocator,
                            size_t                             alignment,
                            size_t                             bytes);
static void   host_free(memory_allocator_t *POUND_RESTRICT allocator, void *pointer);
static size_t host_get_usable_size(const void *POUND_RESTRICT pointer);
static size_t host_get_heap_size(void);

memory_allocator_t g_host_allocator = {
    .allocate         = host_allocate,
    .free             = host_free,
    .get_usable_size  = host_get_usable_size,
    .get_heap_size    = host_get_heap_size,
    .total_bytes_used = 0,
};

POUND_THREAD_LOCAL memory_allocator_t   *tls_current_allocator    = &g_host_allocator;
POUND_THREAD_LOCAL memory_bucket_type_t  tls_current_bucket_index = MEMORY_BUCKET_NONE;
POUND_THREAD_LOCAL memory_bucket_stats_t tls_memory_bucket_stats[MEMORY_BUCKET_COUNT] = { 0 };

void
memory_subsystem_init(void)
{
    tls_current_allocator = &g_host_allocator;
}

void
memory_subsystem_destroy(void)
{
    tls_current_allocator = NULL;
}

memory_allocator_t *
memory_subsystem_set_allocator(memory_allocator_t *POUND_RESTRICT allocator)
{
    memory_allocator_t *POUND_RESTRICT previous_allocator = tls_current_allocator;
    tls_current_allocator                                 = allocator;
    return previous_allocator;
}

memory_allocator_t *
memory_subsystem_get_allocator(void)
{
    return tls_current_allocator;
}

void *
memory_subsystem_allocate(const size_t alignment, const size_t bytes)
{
    void *POUND_RESTRICT pointer
        = tls_current_allocator->allocate(tls_current_allocator, alignment, bytes);
    return pointer;
}

void
memory_subsystem_free(void *POUND_RESTRICT pointer)
{
    tls_current_allocator->free(tls_current_allocator, pointer);
}

memory_bucket_type_t
memory_subsystem_set_bucket(const memory_bucket_type_t bucket)
{
    const memory_bucket_type_t old_bucket_index = tls_current_bucket_index;

    if (POUND_UNLIKELY(bucket >= MEMORY_BUCKET_COUNT))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Rejecting invalid memory bucket %d; keeping bucket %d.",
                        (int)bucket,
                        (int)old_bucket_index);
        return old_bucket_index;
    }

    tls_current_bucket_index = bucket;
    return old_bucket_index;
}

memory_bucket_stats_t *
memory_subsystem_get_bucket_stats(memory_bucket_type_t bucket)
{
    const int bucket_index = (int)bucket;

    if (POUND_UNLIKELY(bucket_index < 0 || bucket_index >= MEMORY_BUCKET_COUNT))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: invalid memory bucket %d.", bucket_index);
        return NULL;
    }

    memory_bucket_stats_t *POUND_RESTRICT stats
        = &tls_current_allocator->bucket_stats[bucket_index];
    return stats;
}

size_t
memory_subsystem_get_usable_size(const void *POUND_RESTRICT pointer)
{
    const size_t usable_size = tls_current_allocator->get_usable_size(pointer);
    return usable_size;
}

size_t
memory_subsystem_get_bytes_used_by_bucket(const int bucket)
{
    if (bucket < 0)
    {
        return tls_current_allocator->total_bytes_used;
    }

    if (POUND_UNLIKELY(bucket >= MEMORY_BUCKET_COUNT))
    {
        POUND_LOG_ERROR(&thread_logger, "Rejecting invalid memory bucket %d.", bucket);
        return 0U;
    }

    const uint64_t used_bytes = tls_current_allocator->bucket_stats[bucket].used_bytes;
    return used_bytes;
}

size_t
memory_subsystem_get_heap_size(void)
{
    const size_t heap_size = tls_current_allocator->get_heap_size();
    return heap_size;
}

static void *
host_allocate(memory_allocator_t *POUND_RESTRICT allocator,
              const size_t                       alignment,
              const size_t                       bytes)
{
    (void)allocator;
    void *POUND_RESTRICT pointer = mi_malloc_aligned(bytes, alignment);

    if (POUND_UNLIKELY(pointer != NULL))
    {
        const size_t                          usable_size = mi_malloc_usable_size(pointer);
        memory_bucket_stats_t *POUND_RESTRICT stats
            = &allocator->bucket_stats[tls_current_bucket_index];

        const uint64_t pages_to_add
            = ((uint64_t)usable_size + MEMORY_PAGE_SIZE - 1ULL) >> MEMORY_PAGE_SHIFT;

        stats->used_bytes += usable_size;
        stats->page_count += pages_to_add;
        ++stats->allocation_count;
        allocator->total_bytes_used += usable_size;
    }

    return pointer;
}

static void
host_free(memory_allocator_t *POUND_RESTRICT allocator, void *pointer)
{
    (void)allocator;

    if (POUND_UNLIKELY(NULL == pointer))
    {
        return;
    }

    const size_t                          usable_size = mi_malloc_usable_size(pointer);
    memory_bucket_stats_t *POUND_RESTRICT stats
        = &allocator->bucket_stats[tls_current_bucket_index];

    const uint64_t used_bytes = stats->used_bytes;
    const uint64_t page_count = stats->page_count;
    const uint64_t pages_to_subtract
        = ((uint64_t)usable_size + MEMORY_PAGE_SIZE - 1ULL) >> MEMORY_PAGE_SHIFT;
    const size_t total_bytes_used = allocator->total_bytes_used;

    stats->used_bytes
        = (used_bytes >= (uint64_t)usable_size) ? (used_bytes - (uint64_t)usable_size) : 0ULL;

    stats->page_count = (page_count >= pages_to_subtract) ? (page_count - pages_to_subtract) : 0ULL;

    allocator->total_bytes_used
        = (total_bytes_used >= usable_size) ? (total_bytes_used - usable_size) : 0ULL;

    mi_free(pointer);
}

size_t
host_get_usable_size(const void *pointer)
{
    if (POUND_UNLIKELY(NULL == pointer))
    {
        return 0U;
    }

    const size_t usable_size = mi_usable_size(pointer);
    return usable_size;
}

size_t
host_get_heap_size(void)
{
    mi_stats_t_decl(stats);
    mi_stats_get(&stats);
    return (size_t)stats.reserved.current;
}
