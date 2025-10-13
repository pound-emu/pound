#include "arena.h"
#include "common/passert.h"
#include <cstring>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

namespace pound::host::memory
{
arena_t arena_init(size_t capacity)
{

#ifdef WIN32
    void* const data = ::VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (NULL == data)
    {
        return {0, 0, nullptr};  // Return invalid arena on failure
    }
#else
    void* const data = ::mmap(nullptr, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == data)
    {
        return {0, 0, nullptr};  // Return invalid arena on failure
    }
#endif

    (void)std::memset(data, POISON_PATTERN, capacity);
    memory::arena_t arena = {
        .capacity = capacity,
        .size = 0,
        .data = data,
    };
    return arena;
}

void* arena_allocate(memory::arena_t* arena, const std::size_t size)
{
    PVM_ASSERT(arena != nullptr);
    PVM_ASSERT(arena->size + size <= arena->capacity);
    void* const data = static_cast<uint8_t*>(arena->data) + arena->size;
    arena->size += size;
    return data;
}
void arena_reset(memory::arena_t* arena)
{
    PVM_ASSERT(nullptr != arena);
    PVM_ASSERT(nullptr != arena->data);
    arena->size = 0;
    (void)std::memset(arena->data, POISON_PATTERN, arena->capacity);
}
void arena_free(memory::arena_t* arena)
{
    PVM_ASSERT(arena != nullptr);

#ifdef WIN32
    const int free = VirtualFree(arena->data, 0, MEM_RELEASE);

    PVM_ASSERT(free != 0);

    if (free == 0)
        PVM_ASSERT_MSG(false, "Failed to free arena memory");
#else
    const int free = munmap(arena->data, arena->capacity);

    PVM_ASSERT(free == 0);

    if (free == -1)
        PVM_ASSERT_MSG(false, "Failed to free arena memory");
#endif

    arena->capacity = 0;
    arena->size = 0;
}
}  // namespace pound::host::memory
