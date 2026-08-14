#ifndef BALLISTIC_MEMORY_H
#define BALLISTIC_MEMORY_H

#include "bal_attributes.h"
#include "bal_errors.h"
#include "bal_log.h"
#include "bal_types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    struct bal_allocator_state_s;
    typedef struct bal_allocator_state_s *bal_allocator_handle_t;

    /// Represents an executable memory buffer for W^X operations.
    typedef struct
    {
        /// Pointer used for writing machine code.
        void *rw_pointer;

        /// Pointer used for executing machine code.
        void *rx_pointer;
    } bal_executable_buffer_t;

    /// A function signature for allocating aligned memory.
    ///
    /// Implementations must allocate a block of memory of at least `size` bytes
    /// with an address that is a multiple of `alignment`. The `alignment`
    /// parameter is guaranteed to be a power of two. The `allocator` parameter
    /// provides access to the opaque state registered in [`bal_allocator_t`].
    ///
    /// Returns a pointer to the allocated memory, or `NULL` if the request could
    /// not be fulfilled.
    typedef void *(*bal_allocate_function_t)(bal_allocator_handle_t allocator,
                                             size_t                 alignment,
                                             size_t                 size);

    /// A function signature for releasing memory.
    ///
    /// Implementations must deallocate the memory at `pointer`, which was
    /// previously allocated by the corresponding to allocate function. The `size`
    /// parameter indicates the size of the allocation being freed. Access to the
    /// heap state is provided via `allocator`.
    typedef void (*bal_free_function_t)(bal_allocator_handle_t allocator,
                                        void                  *pointer,
                                        size_t                 size);

    /// A function signature for allocating executable memory supporting W^X.
    ///
    /// Implementations must allocate a block of memory of at least `size` bytes, aligned to
    /// `alignment`. The memory must be configured to allow writing initially.
    ///
    /// # Returns
    ///
    /// [`bal_executable_buffer_t`] containing the mapped pointers. If the allocation
    /// fails, both `rw_pointer` and `rx_pointer` must be set to `NULL`.
    typedef bal_executable_buffer_t (*bal_allocate_executable_function_t)(
        bal_allocator_handle_t allocator, size_t alignment, size_t size);

    ///  Converts the memory buffer to Read/Write (RW).
    typedef void (*bal_protect_rw_function_t)(bal_allocator_handle_t  allocator,
                                              bal_executable_buffer_t buffer,
                                              size_t                  size);

    /// Converts the memory buffer to Read/Execute (RX).
    typedef void (*bal_protect_rx_function_t)(bal_allocator_handle_t  allocator,
                                              bal_executable_buffer_t buffer,
                                              size_t                  size);

    /// A function signature for freeing executable memory.
    ///
    /// Implementations must deallocate the memory in `buffer`.
    typedef void (*bal_free_executable_function_t)(bal_allocator_handle_t  allocator,
                                                   bal_executable_buffer_t buffer,
                                                   size_t                  size);

    /// Translates a Guest Virtual Address (GVA) to a Host Virtual Address (HVA).
    ///
    /// Ballistic invokes this callback when it needs to fetch instructions or
    /// access data.
    ///
    /// Returns a pointer to the host memory containing the data at `guest_address`, or `NULL`
    /// if the address is unmapped or invalid.
    ///
    /// # Safety
    ///
    /// The implementation must translate the provided `guest_address`
    /// using the opaque `context` and return a pointer to the corresponding host
    /// memory.
    ///
    /// The implementation must write the number of contiguous, readable bytes
    /// available at the returned pointer into `max_readable_size_bytes`. This prevents
    /// Ballistic from reading beyond the end of a mapped page or buffer.
    typedef const uint8_t *(*bal_translate_function_t)(void               *context,
                                                       bal_guest_address_t guest_address,
                                                       size_t             *max_readable_size_bytes);

    /// The host application is responsible for providing an allocator capable of
    /// handling aligned memory requests.
    typedef struct
    {
        /// An opaque pointer defining the state or tracking information for the
        /// heap.
        bal_allocator_handle_t context;

        /// The callback invoked to allocate aligned memory.
        ///
        /// # Safety
        ///
        /// The implementation must return memory aligned to at least 64 bytes if
        /// requested.
        bal_allocate_function_t allocate;

        /// The callback to release memory.
        bal_free_function_t free;

        /// Callback to allocate W^X executable memory.
        bal_allocate_executable_function_t allocate_executable;

        /// Callback to free W^X executable memory.
        bal_free_executable_function_t free_executable;

        /// Converts W^X memory to Read/Write.
        bal_protect_rw_function_t protect_rw;

        /// Converts W^X memory to Read/Execute.
        bal_protect_rx_function_t protect_rx;
    } bal_allocator_t;

    /// Defines the interface for translating guest addresses to host memory.
    typedef struct
    {
        /// An opaque pointer to the context required for address translation
        /// (e.g, a page walker or a buffer descriptor.).
        void *context;

        /// The callback invoked to perform address translation.
        bal_translate_function_t translate;
    } bal_memory_interface_t;

    /// Populates `out_allocator` with the default system implementation.
    ///
    /// # Safety
    ///
    /// `out_allocator` must not be `NULL`.
    ///
    /// # Platform Support
    ///
    /// This function only supports windows and POSIX-compliant systems.
    BAL_COLD void bal_allocator_default_init(bal_allocator_t *out_allocator);

    /// Initializes a flat, contiguous translation interface.
    ///
    /// This convenience function sets up a [`bal_memory_interface_t`] where guest
    /// addresses map directly to offsets within the provided host `buffer`.
    ///
    /// The internal interface is allocated with `allocator`. `interface` is
    /// populated with the resulting context and translation callbacks. `buffer`
    /// must be a  pre-allocated block of host memory of at least `size` bytes.
    ///
    /// Returns [`BAL_SUCCESS`] on success.
    ///
    /// # Errors
    ///
    /// Returns [`BAL_ERROR_INVALID_ARGUMENT`] if any pointer are `NULL`.
    ///
    /// Returns [`BAL_ERROR_MEMORY_ALIGNMENT`] if `buffer` is not aligned to a 16-byte
    /// boundary.
    ///
    /// Returns [`BAL_ERROR_ALLOCATION_FAILED`] if failed to allocate memory interface.
    ///
    /// # Safety
    ///
    /// The caller retains ownership of `buffer`. It must remain valid and
    /// unmodified for the lifetime of the created interface.
    BAL_COLD bal_error_t
    bal_flat_translation_interface_init(bal_allocator_t *BAL_RESTRICT        allocator,
                                        bal_memory_interface_t *BAL_RESTRICT interface,
                                        void *BAL_RESTRICT                   buffer,
                                        size_t                               size);

    /// Frees the internal state allocated within `interface` using the provided
    /// `allocator` and resets `interface` to 0 with a memset.
    ///
    /// This does **not** free the buffer passed to [`bal_flat_translation_interface_init`] as
    /// Ballistic does not take ownership of the host memory region.
    BAL_COLD bal_error_t bal_flat_translation_interface_destroy(bal_allocator_t        *allocator,
                                                                bal_memory_interface_t *interface);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BALLISTIC_MEMORY_H */

/*** end of file ***/
