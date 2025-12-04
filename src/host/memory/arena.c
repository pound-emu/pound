#include "arena.h"
#include "common/passert.h"
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

pvm_host_memory_arena_t pvm_host_memory_arena_init(size_t capacity)
{
    pvm_host_memory_arena_t arena = {
        .capacity = capacity,
        .size = 0,
        .data = NULL,
    };
#ifdef WIN32
    void* const data = VirtualAlloc(NULL, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (NULL == data)
    {
        arena.caapcity = 0;
        return arena;
    }
#else
    void* const data = mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == data)
    {
        arena.capacity = 0;
        return arena;
    }
#endif
    
    (void)memset(data, POISON_PATTERN, capacity);
    arena.data = data;
    return arena;
}

void* pvm_host_memory_arena_allocate(pvm_host_memory_arena_t* arena, const size_t size)
{
    PVM_ASSERT(arena != NULL);
    PVM_ASSERT(arena->size + size <= arena->capacity);
    void* const data = ((uint8_t*)arena->data) + arena->size;
    arena->size += size;
    return data;
}
void pvm_host_memory_arena_reset(pvm_host_memory_arena_t* arena)
{
    PVM_ASSERT(NULL != arena);
    PVM_ASSERT(NULL != arena->data);
    arena->size = 0;
    (void)memset(arena->data, POISON_PATTERN, arena->capacity);
}
void pvm_host_memory_arena_free(pvm_host_memory_arena_t* arena)
{
    PVM_ASSERT(NULL != arena);
    PVM_ASSERT(NULL != arena->data);

#ifdef WIN32
    size_t size = 0;
    const int return_val = VirtualFree(arena->data, size, MEM_RELEASE);
    if (0 == return_val)
    {
        PVM_ASSERT_MSG(false, "Failed to free arena memory");
    }
#else
    long page_size = sysconf(_SC_PAGESIZE);
    PVM_ASSERT(page_size > 0);
    PVM_ASSERT(arena->capacity > 0);
    PVM_ASSERT(0 == ((uintptr_t)arena->data % (size_t)page_size));
    int return_val = munmap(arena->data, arena->capacity);
    if (-1 == return_val)
    {
        PVM_ASSERT_MSG(false, "Failed to free arena memory");
    }
#endif

    arena->capacity = 0;
    arena->size = 0;
}
