/**
 *  @file type.h
 *
 *  @brief Defines the type system for the Pound JIT Intermediate Representation.
 *  
 *  This header declares the `type_t ` enumeration, which forms the basis of
 *  type identification and checking within the JIT's IR. 
 */

namespace pound::jit::ir {

/*!
 * @brief Enumerations of all possible types for a value in the JIT's IR.
 *
 * The values are defined as bit flags to allow combinations using bitwise OR
 * operations.
 */
typedef enum
{
    IR_TYPE_VOID        = 0,
    IR_TYPE_U1          = 1 << 0,
    IR_TYPE_U8          = 1 << 1,
    IR_TYPE_U16         = 1 << 2,
    IR_TYPE_U32         = 1 << 3,
    IR_TYPE_U64         = 1 << 4,
    IR_TYPE_U128        = 1 << 5,
#if 0
    IR_TYPE_A32_REG     = 1 << 6, // ARM32 GPR R0-R14
    IR_TYPE_A32_EXT_REG = 1 << 7, // ARM32 Extended Registers (e.g., for
                                  // VFP/NEON, or just R15 if treated specially)
    IR_TYPE_A32_CPSR = 1 << 8,    // ARM32 CPSR/SPSR
    IR_TYPE_COND     = 1 << 9,    // Condition codes
    IR_TYPE_ACC_TYPE = 1 << 10,   // Memory access type
#endif
    IR_TYPE_OPAQUE
    = 1 << 11, // Represents a value defined by another IR instruction
} type_t;

/*!
 * @brief Checks if two IR types are compatible.
 *
 * Compatibility is determined based on the following rules:
 *
 * 1.   IR_TYPE_VOID is only compatible with itself.
 * 2.   IR_TYPE_OPAQUE is only compatible with with itself at this level of
 *      static type checking. Deeper analysis of the instruction's return type
 *      is required which is beyond the scope of this function.
 * 3.   For all other types, t1 and t2 are compatible if they share at least
 *      one common flag (e.g. IR_TYPE_U32 is compatible with
 *      IR_TYPE_U32 | IR_TYPE_U64).
 *
 * @param t1 The first type_t to compare.
 * @param t2 The second type_t to compare.
 *
 * @retval true if t1 and t2 are compatible according to the defined static
 *         compatibility rules. false otherwise.
 */
bool are_types_compatible(const type_t t1, const type_t t2);
} //namespace pound::jit::ir
