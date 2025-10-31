#include "type.h"
#include "common/passert.h"

namespace pound::jit::ir {
bool
are_types_compatible (const type_t t1, const type_t t2)
{
    const bool is_compatible
        = (t1 == t2) || (IR_TYPE_OPAQUE == t1) || (IR_TYPE_OPAQUE == t2);
    return is_compatible;
}
}
