#ifndef POUND_GUEST_STATE_H
#define POUND_GUEST_STATE_H

#include "../core/attributes.h"
#include "../core/errors.h"
#include "backend/bal_cpu.h"
#include "stdint.h"
#include <assert.h>
#include <stddef.h>

#define GUEST_STATE_REGISTER_COUNT 32

POUND_ALIGNED(64) typedef struct
{
    uint64_t x[GUEST_STATE_REGISTER_COUNT];
    uint64_t pc;
    uint8_t  flag_n;
    uint8_t  flag_z;
    uint8_t  flag_c;
    uint8_t  flag_v;
    uint32_t pad;
} guest_state_t;

static_assert(offsetof(guest_state_t, x) == offsetof(bal_cpu_t, x),
              "guest_state_t.x must overlap bal_cpu_t.x");

static_assert(offsetof(guest_state_t, pc) == offsetof(bal_cpu_t, pc),
              "guest_state_t.pc must overlap bal_cpu_t.pc");

error_t guest_state_init(guest_state_t *state);

#endif // POUND_GUEST_STATE_H

/*** end of file ***/