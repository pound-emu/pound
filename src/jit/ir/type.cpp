#include "common/passert.h"

namespace pound::jit::ir {
typedef enum
{
    IR_TYPE_VOID        = 0,
    IR_TYPE_U1          = 1 << 0,
    IR_TYPE_U8          = 1 << 1,
    IR_TYPE_U16         = 1 << 2,
    IR_TYPE_U32         = 1 << 3,
    IR_TYPE_U64         = 1 << 4,
    IR_TYPE_U128        = 1 << 5,
    IR_TYPE_A32_REG     = 1 << 6, // ARM32 GPR R0-R14
    IR_TYPE_A32_EXT_REG = 1 << 7, // ARM32 Extended Registers (e.g., for
                                  // VFP/NEON, or just R15 if treated specially)
    IR_TYPE_A32_CPSR = 1 << 8,    // ARM32 CPSR/SPSR
    IR_TYPE_COND     = 1 << 9,    // Condition codes
    IR_TYPE_ACC_TYPE = 1 << 10,   // Memory access type
    IR_TYPE_OPAQUE
    = 1 << 11, // Represents a value defined by another IR instruction
} ir_type_t;

bool
are_types_compatible (const ir_type_t t1, const ir_type_t t2)
{
    const bool is_compatible
        = (t1 == t2) || (IR_TYPE_OPAQUE == t1) || (IR_TYPE_OPAQUE == t2);
    return is_compatible;
}
}
