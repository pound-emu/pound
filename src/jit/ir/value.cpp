#include "value.h"
#include "common/passert.h"
#include <stdint.h>

namespace pound::jit::ir {
/*
 * ============================================================================
 *                              Init Functions
 * ============================================================================
 */

void
value_init (value_t *p_value)
{
    PVM_ASSERT(nullptr != p_value);
    p_value->type = IR_TYPE_VOID;
}

void
value_init_from_u64 (value_t *p_value, const uint64_t u64)
{
    PVM_ASSERT(nullptr != p_value);
    p_value->type                = IR_TYPE_U64;
    p_value->inner.immediate_u64 = u64;
}

void
value_init_from_u32 (value_t *p_value, const uint32_t u32)
{
    PVM_ASSERT(nullptr != p_value);
    p_value->type                = IR_TYPE_U32;
    p_value->inner.immediate_u32 = u32;
}

void
value_init_from_u8 (value_t *p_value, const uint8_t u8)
{
    PVM_ASSERT(nullptr != p_value);
    p_value->type               = IR_TYPE_U8;
    p_value->inner.immediate_u8 = u8;
}

void
value_init_from_u1 (value_t *p_value, const bool u1)
{
    PVM_ASSERT(nullptr != p_value);
    p_value->type               = IR_TYPE_U1;
    p_value->inner.immediate_u1 = u1;
}

void
value_init_from_a32_register (value_t *p_value, const a32_register_t reg)
{
    PVM_ASSERT(nullptr != p_value);
    p_value->type = IR_TYPE_A32_REGISTER;
    p_value->inner.immediate_a32_register = reg;
}

/*
 * ============================================================================
 *                              Getter Functions
 * ============================================================================
 */

const uint64_t
value_get_u64 (const value_t *p_value)
{ 
    PVM_ASSERT(IR_TYPE_U64 == p_value->type);
    return p_value->inner.immediate_u64;
}

const uint32_t
value_get_u32 (const value_t *p_value)
{
    PVM_ASSERT(IR_TYPE_U32 == p_value->type);
    return p_value->inner.immediate_u32;
}

const uint8_t
value_get_u8 (const value_t *p_value)
{
    PVM_ASSERT(IR_TYPE_U8 == p_value->type);
    return p_value->inner.immediate_u8;
}

const bool
value_get_u1 (const value_t *p_value)
{
    PVM_ASSERT(IR_TYPE_U1 == p_value->type);
    return p_value->inner.immediate_u1;
}

const pound::jit::a32_register_t
value_get_a32_register (const value_t *p_value)
{
    PVM_ASSERT(IR_TYPE_A32_REGISTER == p_value->type);
    return p_value->inner.immediate_a32_register;
}
}
