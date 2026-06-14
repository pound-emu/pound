#ifndef POUND_GPU_SPIRV_IR_H
#define POUND_GPU_SPIRV_IR_H
#include "generated/opcodes.h"
#include <stdint.h>

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

typedef struct
{
    sm86_opcode_t *opcodes;
    uint32_t      *predicates;

    uint32_t *destination0; // GPR register 1.
    uint32_t *destination1; // GPR register 2 (TEX instructions) OR Predicate
    uint32_t *destination2; // Predicate
    uint32_t *source0;
    uint32_t *source1;
    uint32_t *source2;
    uint32_t  capacity;
    uint32_t  count;
} ir_context_t;

#endif // POUND_GPU_SPIRV_IR_H

/*** end of file ***/