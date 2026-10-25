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
    void *(*allocate)(memory_allocator_t *POUND_RESTRICT allocator, size_t alignment, size_t bytes);
    void (*free)(memory_allocator_t *POUND_RESTRICT allocator, void *pointer);
    size_t (*get_usable_size)(const void *POUND_RESTRICT pointer);
    size_t (*get_heap_size)(void);
    size_t bytes_used_by_bucket[MEMORY_BUCKET_COUNT];
    size_t total_bytes_used;
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

POUND_THREAD_LOCAL memory_allocator_t  *tls_current_allocator    = &g_host_allocator;
POUND_THREAD_LOCAL memory_bucket_type_t tls_current_bucket_index = MEMORY_BUCKET_NONE;

// Tracks which bucket each live allocation was charged to, so that
// host_free() can debit the same bucket even if the active bucket
// changed between allocation and free (otherwise bytes_used_by_bucket[]
// underflows).
typedef struct
{
    void  *pointer;
    size_t bucket;
} allocation_record_t;

static allocation_record_t *allocation_records        = NULL;
static size_t              allocation_record_count    = 0;
static size_t              allocation_record_capacity = 0;

#define ALLOCATION_BUCKET_UNKNOWN ((size_t)-1)

static void record_allocation(void *pointer, size_t bucket);
static size_t lookup_and_remove_allocation_bucket(void *pointer);

void
memory_subsystem_init(void)
{
    tls_current_allocator = &g_host_allocator;
    for (size_t i = 0; i < MEMORY_BUCKET_COUNT; ++i)
    {
        g_host_allocator.bytes_used_by_bucket[i] = 0;
    }
    g_host_allocator.total_bytes_used = 0;

    free(allocation_records);
    allocation_records        = NULL;
    allocation_record_count   = 0;
    allocation_record_capacity = 0;
}

void
memory_subsystem_destroy(void)
{
    free(allocation_records);
    allocation_records        = NULL;
    allocation_record_count   = 0;
    allocation_record_capacity = 0;

    for (size_t i = 0; i < MEMORY_BUCKET_COUNT; ++i)
    {
        g_host_allocator.bytes_used_by_bucket[i] = 0;
    }
    g_host_allocator.total_bytes_used = 0;

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

    const size_t bytes_used_by_bucket = tls_current_allocator->bytes_used_by_bucket[bucket];
    return bytes_used_by_bucket;
}

size_t
memory_subsystem_get_heap_size(void)
{
    const size_t heap_size = tls_current_allocator->get_heap_size();
    return heap_size;
}

static void
record_allocation(void *pointer, size_t bucket)
{
    if (ALLOCATION_BUCKET_UNKNOWN == bucket)
    {
        return;
    }

    if (allocation_record_count == allocation_record_capacity)
    {
        const size_t new_capacity = (0U == allocation_record_capacity)
                                        ? 64U
                                        : allocation_record_capacity * 2U;
        allocation_record_t *new_records
            = realloc(allocation_records, new_capacity * sizeof(allocation_record_t));

        if (NULL == new_records)
        {
            // Tracking failed; accounting for this allocation will be
            // best-effort only. Do not crash.
            return;
        }

        allocation_records        = new_records;
        allocation_record_capacity = new_capacity;
    }

    allocation_records[allocation_record_count].pointer = pointer;
    allocation_records[allocation_record_count].bucket  = bucket;
    ++allocation_record_count;
}

static size_t
lookup_and_remove_allocation_bucket(void *pointer)
{
    for (size_t i = 0; i < allocation_record_count; ++i)
    {
        if (allocation_records[i].pointer == pointer)
        {
            const size_t bucket = allocation_records[i].bucket;

            // Remove by swapping with the last record (O(1)).
            allocation_records[i] = allocation_records[allocation_record_count - 1U];
            --allocation_record_count;

            return bucket;
        }
    }

    // Unknown allocation: cannot attribute it to a bucket.
    return ALLOCATION_BUCKET_UNKNOWN;
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
        const size_t usable_size = mi_malloc_usable_size(pointer);
        allocator->bytes_used_by_bucket[tls_current_bucket_index] += usable_size;
        allocator->total_bytes_used += usable_size;
        record_allocation(pointer, tls_current_bucket_index);
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

    const size_t usable_size = mi_malloc_usable_size(pointer);
    const size_t bucket      = lookup_and_remove_allocation_bucket(pointer);

    if (ALLOCATION_BUCKET_UNKNOWN != bucket
        && bucket < MEMORY_BUCKET_COUNT)
    {
        allocator->bytes_used_by_bucket[bucket] -= usable_size;
    }
    allocator->total_bytes_used -= usable_size;
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
