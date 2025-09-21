#include "passert.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define ASSERT_MESSAGE_BUFFER_SIZE 1024

void pound_internal_assert_fail(const char* file, int line, const char* func, const char* expr_str,
                                const char* user_msg, ...)
{
    char assert_format[] =
        " \
        ================================================================================ \n \
        PVM ASSERTION FAILURE \n \
        ================================================================================ \n \
        File:         %s \n \
        Line:         %d \n \
        Function:     %s \n \
        Expression:   %s \n \
        Message:      %s \n \
        ================================================================================ \n \
        Terminating program via abort(). Core dump expected. \n \
        ";

    char message_str[ASSERT_MESSAGE_BUFFER_SIZE] = {};
    if (nullptr == user_msg)
    {
        (void)strcpy(message_str, "n/a");
    }
    else
    {
        va_list args;
        va_start(args, user_msg);
        (void)vsnprintf(message_str, ASSERT_MESSAGE_BUFFER_SIZE, user_msg, args);
        va_end(args);
    }

    char buffer[ASSERT_MESSAGE_BUFFER_SIZE] = {};
    (void)snprintf(buffer, ASSERT_MESSAGE_BUFFER_SIZE, assert_format, file, line, func, expr_str, message_str);

    (void)fprintf(stderr, "%s", buffer);
    abort();
}
