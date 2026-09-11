#include "memory.h"
#include <stdlib.h>

static void *host_allocate(memory_allocator_t *POUND_RESTRICT allocator, size_t size);
static void  host_free(memory_allocator_t *POUND_RESTRICT allocator, void *pointer);

memory_allocator_t g_host_allocator = { .allocate = host_allocate, .free = host_free };
POUND_THREAD_LOCAL memory_allocator_t *tls_current_allocator = &g_host_allocator;

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

static void *
host_allocate(memory_allocator_t *allocator, const size_t size)
{
    (void)allocator;
    void *POUND_RESTRICT pointer = malloc(size);
    return pointer;
}

static void
host_free(memory_allocator_t *POUND_RESTRICT allocator, void *pointer)
{
    (void)allocator;
    free(pointer);
}
