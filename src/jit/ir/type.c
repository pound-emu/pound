#include "type.h"
#include "common/passert.h"

bool
pvm_jit_ir_are_types_compatible (const pvm_jit_ir_type_t t1,
                                 const pvm_jit_ir_type_t t2)
{
    const bool is_compatible
        = (t1 == t2) || (IR_TYPE_OPAQUE == t1) || (IR_TYPE_OPAQUE == t2);
    return is_compatible;
}
