#ifndef POUND_GPU_RECOMPILER_IR_H
#define POUND_GPU_RECOMPILER_IR_H

typedef enum
{
    IR_TYPE_VOID = 0,
    IR_TYPE_U32,
    IR_TYPE_I32,
    IR_TYPE_F32,
    IR_TYPE_PREDICATE, // 1-bit boolean
} ir_operand_type_t;

typedef enum
{
    IR_OPCODE_IADD,
    IR_OPCODE_LOAD_GPR,
    IR_OPCODE_STORE_GPR,
    IR_OPCODE_BRANCH,
    IR_OPCODE_EXIT,
} ir_opcode_t;

#endif // POUND_GPU_RECOMPILER_IR_H
