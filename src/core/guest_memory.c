#include "guest_memory.h"

error_t
guest_memory_init(guest_memory_t *guest_memory,
                  void           *host_base,
                  const size_t    host_size,
                  const uint64_t  guest_base)
{
    if (guest_memory == NULL || host_base == NULL || host_size == 0)
    {
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    const uintptr_t host_address = (uintptr_t)host_base;

    if ((host_address & (uintptr_t)15) != (uintptr_t)0)
    {
        return POUND_ERROR_MEMORY_ALIGNMENT;
    }

    if (guest_base > UINT64_MAX - host_size)
    {
        return POUND_ERROR_GUEST_ADDRESS_OVERFLOW;
    }

    guest_memory->host_base  = host_base;
    guest_memory->host_size  = host_size;
    guest_memory->guest_base = guest_base;
    guest_memory->guest_end  = guest_base + host_size;
    return POUND_SUCCESS;
}

bool
guest_memory_contains(const guest_memory_t *guest_memory, const uint64_t guest_address)
{
    if (guest_memory == NULL)
    {
        return false;
    }

    const uint64_t guest_base = guest_memory->guest_base;
    const uint64_t guest_end  = guest_memory->guest_end;
    const bool     contains   = guest_address >= guest_base && guest_address < guest_end;
    return contains;
}

const uint8_t *
guest_memory_translate_read(const guest_memory_t *guest_memory,
                            const uint64_t        guest_address,
                            size_t               *max_readable_bytes)
{
    if (NULL == guest_memory || NULL == max_readable_bytes)
    {
        return NULL;
    }

    const bool is_guest_address_valid = guest_memory_contains(guest_memory, guest_address);

    if (false == is_guest_address_valid)
    {
        return NULL;
    }

    const uint64_t offset    = guest_address - guest_memory->guest_base;
    const uint64_t host_size = guest_memory->host_size;

    if (offset >= guest_memory->host_size)
    {
        return NULL;
    }

    *max_readable_bytes                       = host_size - offset;
    const uint8_t *POUND_RESTRICT host_base   = guest_memory->host_base;
    const uint8_t *POUND_RESTRICT host_offset = host_base + offset;
    return host_offset;
}

uint8_t *
guest_memory_translate_write(const guest_memory_t *POUND_RESTRICT guest_memory,
                             const uint64_t                       guest_address,
                             size_t *POUND_RESTRICT               max_writable_bytes)
{
    if (NULL == guest_memory || NULL == max_writable_bytes)
    {
        return NULL;
    }

    const bool is_guest_address_valid = guest_memory_contains(guest_memory, guest_address);

    if (false == is_guest_address_valid)
    {
        return NULL;
    }

    const uint64_t offset    = guest_address - guest_memory->guest_base;
    const uint64_t host_size = guest_memory->host_size;

    if (offset >= guest_memory->host_size)
    {
        return NULL;
    }

    *max_writable_bytes                 = host_size - offset;
    uint8_t *POUND_RESTRICT host_base   = guest_memory->host_base;
    uint8_t *POUND_RESTRICT host_offset = host_base + offset;
    return host_offset;
}

/*** end of file ***/
