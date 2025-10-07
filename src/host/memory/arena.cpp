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
// new more memsafe code (ownedbywuigi) (i give up on windows compatibility for now, will stick to the old unsafe code)

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
    arena->capacity = 0;
    arena->size = 0;

    // TODO(GloriousTaco:memory): Replace free with a memory safe alternative.
#ifdef WIN32
    VirtualFree(arena->data, 0, MEM_RELEASE);
#else
    free(arena->data);
#endif
}
}  // namespace pound::host::memory
