#ifndef POUND_GUEST_STATE_H
#define POUND_GUEST_STATE_H

#include "attributes.h"
#include "errors.h"
#include "stdint.h"

#define GUEST_STATE_REGISTER_COUNT 32

POUND_ALIGNED(64) typedef struct
{
    uint64_t x[GUEST_STATE_REGISTER_COUNT];
    uint64_t pc;
    uint8_t  flag_n;
    uint8_t  flag_z;
    uint8_t  flag_c;
    uint8_t  flag_v;
} guest_state_t;

error_t guest_state_init(guest_state_t *state);

#endif // POUND_GUEST_STATE_H

/*** end of file ***/