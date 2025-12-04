#include "value.h"
#include "common/passert.h"
#include <stdint.h>

/*
 * ============================================================================
 *                              Init Functions
 * ============================================================================
 */

void
pvm_jit_ir_value_init (pvm_jit_ir_value_t *value)
{
    PVM_ASSERT(NULL != value);
    value->type = IR_TYPE_VOID;
}

void
pvm_jit_ir_value_init_from_u64 (pvm_jit_ir_value_t *value, const uint64_t u64)
{
    PVM_ASSERT(NULL != value);
    value->type                = IR_TYPE_U64;
    value->inner.immediate_u64 = u64;
}

void
pvm_jit_ir_value_init_from_u32 (pvm_jit_ir_value_t *value, const uint32_t u32)
{
    PVM_ASSERT(NULL != value);
    value->type                = IR_TYPE_U32;
    value->inner.immediate_u32 = u32;
}

void
pvm_jit_ir_value_init_from_u8 (pvm_jit_ir_value_t *value, const uint8_t u8)
{
    PVM_ASSERT(NULL != value);
    value->type               = IR_TYPE_U8;
    value->inner.immediate_u8 = u8;
}

void
pvm_jit_ir_value_init_from_u1 (pvm_jit_ir_value_t *value, const bool u1)
{
    PVM_ASSERT(NULL != value);
    value->type               = IR_TYPE_U1;
    value->inner.immediate_u1 = u1;
}

void
pvm_jit_ir_value_init_from_a32_register (pvm_jit_ir_value_t          *value,
                                         const pvm_jit_a32_register_t reg)
{
    PVM_ASSERT(NULL != value);
    value->type                         = IR_TYPE_A32_REGISTER;
    value->inner.immediate_a32_register = reg;
}

/*
 * ============================================================================
 *                              Getter Functions
 * ============================================================================
 */

uint64_t
pvm_jit_ir_value_get_u64 (const pvm_jit_ir_value_t *value)
{
    PVM_ASSERT(IR_TYPE_U64 == value->type);
    return value->inner.immediate_u64;
}

uint32_t
pvm_jit_ir_value_get_u32 (const pvm_jit_ir_value_t *value)
{
    PVM_ASSERT(IR_TYPE_U32 == value->type);
    return value->inner.immediate_u32;
}

uint8_t
pvm_jit_ir_value_get_u8 (const pvm_jit_ir_value_t *value)
{
    PVM_ASSERT(IR_TYPE_U8 == value->type);
    return value->inner.immediate_u8;
}

bool
pvm_jit_ir_value_get_u1 (const pvm_jit_ir_value_t *value)
{
    PVM_ASSERT(IR_TYPE_U1 == value->type);
    return value->inner.immediate_u1;
}

pvm_jit_a32_register_t
pvm_jit_ir_value_get_a32_register (const pvm_jit_ir_value_t *value)
{
    PVM_ASSERT(IR_TYPE_A32_REGISTER == value->type);
    return value->inner.immediate_a32_register;
}
