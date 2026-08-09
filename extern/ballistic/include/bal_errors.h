/** @file bal_errors.h
 *
 * @brief Defines Ballistic error types.
 */

#ifndef BAL_ERRORS_H
#define BAL_ERRORS_H

#include "bal_attributes.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef enum
    {
        // General Errors.

        /// Operation completed successfully.
        BAL_SUCCESS = 0,

        /// A function argument was invalid.
        BAL_ERROR_INVALID_ARGUMENT = -1,

        /// A memory allocation failed.
        BAL_ERROR_ALLOCATION_FAILED = -2,

        /// A struct integrity check failed. See [`bal_safety.h`].
        BAL_ERROR_STRUCT_CORRUPTED = -3,

        /// An internal buffer is full.
        BAL_ERROR_BUFFER_OVERFLOW = -4,

        // Memory and Translation Errors

        /// Buffer is not aligned to the required memory alignment.
        BAL_ERROR_MEMORY_ALIGNMENT = -50,

        /// Ballistic tried to access memory it was not allowed to access.
        BAL_ERROR_MEMORY_FAULT = -51,

        /// Engine and Execution Errors

        /// The engine is already executing guest code.
        BAL_ERROR_ENGINE_ALREADY_RUNNING = -100,

        /// Guest code tried to execute an unaligned instruction.
        BAL_ERROR_PC_ALIGNMENT = -101,

        /// Ballistic tried to access memory it was not allowed to access.
        BAL_ERROR_UNKNOWN_INSTRUCTION = -102,

        // Thread Errors

        /// Error when creating a thread.
        BAL_ERROR_THREAD_CREATION = -150,

        /// Error on thread clean up. The user will need to manually clean up the thread's
        /// resources.
        BAL_ERROR_THREAD_CLEANUP = -151,

        // IR / Assembler Errors.

        /// The instruction buffer is full.
        BAL_ERROR_INSTRUCTION_OVERFLOW = -200,

        /// The decoded register type does not match the expected type.
        BAL_ERROR_INCORRECT_REGISTER_TYPE = -201,

        /// The relative jump or branch offset exceeded the capacity of the target displacement
        /// field.
        BAL_ERROR_BRANCH_OFFSET_OVERFLOW = -202,

        /// The provided buffer capacity is too large and would cause an integer overflow.
        BAL_ERROR_CAPACITY_TOO_BIG = -203,
    } bal_error_t;

    /// Converts the enum into a readable string for error handling.
    BAL_COLD const char *bal_error_to_string(bal_error_t error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BAL_ERRORS_H */

/*** end of file ***/
