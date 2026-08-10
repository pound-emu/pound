#include "guest_state.h"

#include <stddef.h>
#include <string.h>

error_t
guest_state_init(guest_state_t *POUND_RESTRICT state)
{
    if (NULL == state)
    {
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    memset(state, 0, sizeof(guest_state_t));
    return POUND_SUCCESS;
}

/*** end of file ***/
