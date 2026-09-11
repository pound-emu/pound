#include "memory.h"
#include "mimalloc.h"
#include <stdlib.h>

static void *host_allocate(memory_allocator_t *POUND_RESTRICT allocator,
                           size_t                             alignment,
                           size_t                             bytes);
static void  host_free(memory_allocator_t *POUND_RESTRICT allocator, void *pointer);

memory_allocator_t g_host_allocator = { .allocate = host_allocate, .free = host_free };
POUND_THREAD_LOCAL memory_allocator_t  *tls_current_allocator    = &g_host_allocator;
POUND_THREAD_LOCAL memory_bucket_type_t tls_current_bucket_index = MEMORY_BUCKET_NONE;

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
    tls_current_bucket_index = (memory_bucket_type_t)(bucket & MEMORY_BUCKET_COUNT);
    return old_bucket_index;
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
        allocator->memory_used_by_bucket[tls_current_bucket_index]
            += mi_malloc_usable_size(pointer);
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

    allocator->memory_used_by_bucket[tls_current_bucket_index] -= mi_malloc_usable_size(pointer);
    mi_free(pointer);
}
