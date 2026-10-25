#include "guest_memory.h"
#include "log.h"
#include "memory/memory.h"
#include <string.h>

error_t
guest_memory_init(guest_memory_t *guest_memory, const size_t host_size, const uint64_t guest_base)
{
    if (POUND_UNLIKELY(NULL == guest_memory))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: guest_memory is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(0 == host_size))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: host_size is 0.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(guest_base > UINT64_MAX - host_size))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: guest_base (%llx) is too big.",
                        (unsigned long long)guest_base);
        return POUND_ERROR_GUEST_ADDRESS_OVERFLOW;
    }

    const memory_bucket_type_t previous_bucket
        = memory_subsystem_set_bucket(MEMORY_BUCKET_GUEST_MEMORY);
    const size_t memory_alignment   = 16U;
    const size_t guest_memory_bytes = host_size;
    void        *host_base = memory_subsystem_allocate(memory_alignment, guest_memory_bytes);
    (void)memory_subsystem_set_bucket(previous_bucket);

    if (POUND_UNLIKELY(NULL == host_base))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: guest_memory allocation failed.");
        return POUND_ERROR_ALLOCATION_FAILED;
    }

    guest_memory->host_base  = host_base;
    guest_memory->host_size  = host_size;
    guest_memory->guest_base = guest_base;
    guest_memory->guest_end  = guest_base + host_size;
    return POUND_SUCCESS;
}

error_t
guest_memory_destroy(guest_memory_t *guest_memory)
{
    if (POUND_UNLIKELY(NULL == guest_memory))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: guest_memory is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(NULL == guest_memory->host_base))
    {
        POUND_LOG_WARN(&thread_logger,
                       "guest_memory->host_base is NULL. Struct was never "
                       "initialized or is already destroyed, resetting it anyways.");
        (void)memset(guest_memory, 0, sizeof(guest_memory_t));
        return POUND_SUCCESS;
    }

    const size_t guest_range_size = guest_memory->guest_end - guest_memory->guest_base;

    if (POUND_UNLIKELY(guest_range_size != guest_memory->host_size))
    {
        POUND_LOG_WARN(&thread_logger,
                       "Guest range size (%zu) differs from host_size (%zu); struct was "
                       "modified after guest_memory_init().",
                       guest_range_size,
                       guest_memory->host_size);
    }

    const memory_bucket_type_t previous_bucket
        = memory_subsystem_set_bucket(MEMORY_BUCKET_GUEST_MEMORY);
    memory_subsystem_free(guest_memory->host_base);
    (void)memory_subsystem_set_bucket(previous_bucket);

    (void)memset(guest_memory, 0, sizeof(guest_memory_t));
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
