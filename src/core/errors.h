#ifndef POUND_ERRORS_H
#define POUND_ERRORS_H

typedef enum
{
    // General Errors

    POUND_SUCCESS,
    POUND_ERROR_INVALID_ARGUMENT,

    // Memory Errors

    /// Buffer is not aligned to the required memory alignment.
    POUND_ERROR_MEMORY_ALIGNMENT,

    /// Tried to access memory it was not allowed to access.
    POUND_ERROR_MEMORY_FAULT,

    /// Guest Address Size is way to big and caused an overflow.
    POUND_ERROR_GUEST_ADDRESS_OVERFLOW,

    /// Guest Address fell outside the mapped memory region.
    POUND_ERROR_GUEST_ADDRESS_OUT_OF_BOUNDS,
} error_t;

#endif // POUND_ERRORS_H

/*** end of file ***/