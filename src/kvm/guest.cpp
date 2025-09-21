#include "guest.h"
#include "common/passert.h"

namespace pound::kvm::memory
{
guest_memory_t* guest_memory_create(pound::host::memory::arena_t* arena)
{
    PVM_ASSERT(nullptr != arena);
    PVM_ASSERT(nullptr != arena->data);

    guest_memory_t* memory = (guest_memory_t*)pound::host::memory::arena_allocate(arena, sizeof(guest_memory_t));
    size_t ram_size = arena->capacity - arena->size;
    uint8_t* ram_block = (uint8_t*)pound::host::memory::arena_allocate(arena, ram_size);

    /* 
     * This requires casting away the 'const' qualifier, which is generally unsafe.
     * However, it is safe in this specific context because:
     *
     *     a) We are operating on a newly allocated heap object (`memory`).
     *     b) No other part of the system has a reference to this object yet.
     *     c) This is a one-time initialization; the const contract will be
     *        honored for the rest of the object's lifetime after this function
     *        returns.
     * 
     * This allows us to create an immutable descriptor object on the
     * heap.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    *(uint8_t**)&memory->base = ram_block;
    *(uint64_t*)&memory->size = ram_size;
#pragma GCC diagnostic pop
    return memory;
}
}  // namespace pound::kvm::memory
