#ifndef POUND_ARENA_ALLOCATOR_H
#define POUND_ARENA_ALLOCATOR_H

#include "arena.h"
#include <cstddef>
#include <memory>

namespace pound::host::memory
{
/**
    @brief An STL-compatible allocator that uses a custom arena for memory management.
    
    This allocator allows STL containers (such as std::vector) to allocate memory from a user-provided arena,
    enabling efficient bulk allocation and deallocation patterns. The arena must provide an `arena_allocate` function.
    
    @tparam T Type of object to allocate.
    
    @code
    arena_t my_arena = memory::arena_init(4096);
    arena_allocator<int> alloc(&my_arena);
    
    std::vector<int, arena_allocator<int>> vec(alloc);
    vec.push_back(42);
    // ...
    arena_reset(&my_arena); // Frees all allocations in the arena
    @endcode
    
    @note The deallocate function is a no-op; memory is managed by the arena.
    
    @see arena_t
    @see arena_allocate
    */
template <typename T>
struct arena_allocator
{
    using value_type = T;

    arena_t* arena;

    arena_allocator(arena_t* a) noexcept : arena(a) {}

    template <typename U>
    arena_allocator(const arena_allocator<U>& other) noexcept : arena(other.arena)
    {
    }

    T* allocate(std::size_t n) { return static_cast<T*>(const_cast<void*>(arena_allocate(arena, n * sizeof(T)))); }

    void deallocate(T*, std::size_t) noexcept
    {
        // noop since memory should be freed by arena
    }

    template <typename U>
    struct rebind
    {
        using other = arena_allocator<U>;
    };
};

template <typename T, typename U>
inline constexpr bool operator==(const arena_allocator<T>& a, const arena_allocator<U>& b)
{
    return a.arena == b.arena;
}

template <typename T, typename U>
inline constexpr bool operator!=(const arena_allocator<T>& a, const arena_allocator<U>& b)
{
    return a.arena != b.arena;
}

}  // namespace pound::host::memory

#endif  // POUND_ARENA_ALLOCATOR_H
