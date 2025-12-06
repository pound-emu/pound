/*
 * THIS FILE IS A WORK IN PROGRESS AND WILL BE RE-WRITTEN
 * Defines pvm_jit_interpreter_instruction_t struct and its internal opcodes.
 */

#include "frontend/decoder/arm32_opcodes.h"
#include <stdlib.h>
#if defined(__GNUC__) || defined(__clang__)
    #define HANDLER(name)   HANDLER_##name
    #define DISPATCH()      do { \
                                instr++; \
                                goto *dispatch_table[instr->opcode]; \
                            } while(0)
#else
    #define HANDLER(name)   case name
    #define DISPATCH()      goto dispatch_loop
#endif

typedef struct
{
    pvm_jit_decoder_arm32_opcode_t opcode;
} instruction_t;

void
temp(void)
{
    instruction_t *instr = malloc(sizeof(*instr));
    instr->opcode = PVM_A32_OP_STOP;
#if defined(__GNUC__) || defined(__clang__)
    static const void* const dispatch_table[] = {
        #include "handler_table.inc"
    };

    // Initial dispatch
    goto *dispatch_table[instr->opcode];
#else
    dispatch_loop:
    switch (instr->opcode) {
#endif

    #include "handlers.inc"

#if !defined(__GNUC__) && !defined(__clang__)
    default: goto HANDLER_PVM_A32_OP_STOP;
    }
#endif
}
