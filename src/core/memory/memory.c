#include "memory.h"
#include <stdlib.h>

static void *host_allocate(memory_allocator_t *POUND_RESTRICT allocator,
                           size_t                             alignment,
                           size_t                             bytes);
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

memory_allocator_t *
memory_subsystem_set_allocator(memory_allocator_t *POUND_RESTRICT allocator)
{
    memory_allocator_t *POUND_RESTRICT previous_allocator = tls_current_allocator;
    tls_current_allocator                                 = allocator;
    return previous_allocator;
}

static void *
host_allocate(memory_allocator_t *POUND_RESTRICT allocator,
              const size_t                       alignment,
              const size_t                       bytes)
{
    (void)allocator;
    (void)alignment;
    void *POUND_RESTRICT pointer = malloc(bytes);
    return pointer;
}

static void
host_free(memory_allocator_t *POUND_RESTRICT allocator, void *pointer)
{
    (void)allocator;
    free(pointer);
}
