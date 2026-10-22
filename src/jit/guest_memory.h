#ifndef POUND_GUEST_MEMORY_H
#define POUND_GUEST_MEMORY_H

#include "attributes.h"
#include "errors.h"
#include "stddef.h"
#include "stdint.h"
#include <assert.h>
#include <stdbool.h>

/// Guest addresses in the range:
///
///     [guest_base, guest_base + size)
///
/// map directly to:
///     host_base + (guest_address - guest_base)
POUND_ALIGNED(64) typedef struct
{
    uint8_t *host_base;
    size_t   host_size;

    /// First valid guest address.
    size_t guest_base;

    /// One past the last valid guest address.
    size_t guest_end;

    char pad[32];
} guest_memory_t;

static_assert(64 == sizeof(guest_memory_t), "Struct size mismatch");

/// `host_base` must be at least 16-byte aligned.
error_t guest_memory_init(guest_memory_t *POUND_RESTRICT guest_memory,
                          void *POUND_RESTRICT           host_base,
                          size_t                         host_size,
                          uint64_t                       guest_base);

/// Returns true if `guest_address` is inside this guest memory region.
bool guest_memory_contains(const guest_memory_t *POUND_RESTRICT guest_memory,
                           uint64_t                             guest_address);

/// Translates a guest address to a host read pointer.
///
/// On success it writes the number of contiguous readable bytes starting at the returned pointer
/// into `max_readable_bytes`.
const uint8_t *guest_memory_translate_read(const guest_memory_t *POUND_RESTRICT guest_memory,
                                           uint64_t                             guest_address,
                                           size_t *POUND_RESTRICT               max_readable_bytes);

/// Translates a guest address to a host read pointer.
///
/// On success it writes the number of contiguous writable bytes starting at the returned pointer
/// into `max_writable_bytes`.
uint8_t *guest_memory_translate_write(const guest_memory_t *POUND_RESTRICT guest_memory,
                                      uint64_t                             guest_address,
                                      size_t *POUND_RESTRICT               max_writable_bytes);

#endif // POUND_GUEST_MEMORY_H
