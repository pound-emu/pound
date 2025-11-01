#include "type.h"
#include "common/passert.h"
#include <stdint.h>

namespace pound::jit::ir 
{
typedef struct
{
    type_t type;
    union
    {
        uint64_t immediate_u64;
        uint32_t immediate_u32;
        uint8_t  immediate_u8;
        bool     immediate_u1;
    } inner;
} value_t;

void
value_init (value_t *value)
{
    PVM_ASSERT(nullptr != value);
    value->type = IR_TYPE_VOID;
}

void
value_init_from_u64 (value_t *value, uint64_t u64)
{
    PVM_ASSERT(nullptr != value);
    value->type                = IR_TYPE_U64;
    value->inner.immediate_u64 = u64;
}

void
value_init_from_u32 (value_t *value, uint32_t u32)
{
    PVM_ASSERT(nullptr != value);
    value->type                = IR_TYPE_U32;
    value->inner.immediate_u32 = u32;
}

void
value_init_from_u8 (value_t *value, uint8_t u8)
{
    PVM_ASSERT(nullptr != value);
    value->type               = IR_TYPE_U8;
    value->inner.immediate_u8 = u8;
}

void
value_init_from_u1 (value_t *value, bool u1)
{
    PVM_ASSERT(nullptr != value);
    value->type               = IR_TYPE_U1;
    value->inner.immediate_u1 = u1;
}
}
