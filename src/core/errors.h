#ifndef POUND_ERRORS_H
#define POUND_ERRORS_H

typedef enum
{
    // General Errors

    POUND_SUCCESS,
    POUND_ERROR_INVALID_ARGUMENT = -1,

    // Memory Errors

    /// Buffer is not aligned to the required memory alignment.
    POUND_ERROR_MEMORY_ALIGNMENT = -50,

    /// Tried to access memory it was not allowed to access.
    POUND_ERROR_MEMORY_FAULT = -51,
} error_t;

#endif // POUND_ERRORS_H

/*** end of file ***/