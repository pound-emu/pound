/*
 * Defines pvm_jit_interpreter_arm32_instruction_t struct and its internal
 * opcodes.
 */

#include "instruction.h"
#include "common/passert.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Computed gotos are a GCC/Clang extension that significantly improves
 * interpreter performance by predicting branch targets.
 */
#if defined(__GNUC__) || defined(__clang__)
#define PVM_USE_COMPUTED_GOTO 1
#else
#define PVM_USE_COMPUTED_GOTO 0
#endif

typedef struct
{
    uint32_t gpr[16];
    uint32_t pstate;
} pvm_jit_interpreter_arm32_cpu_state_t;

void
temp (void)
{
    pvm_jit_interpreter_arm32_instruction_t *instr = calloc(3, sizeof(*instr));
    instr->opcode                                  = PVM_A32_OP_ADD_REG;
    (++instr)->opcode                              = PVM_A32_OP_STOP;
/*
 * Uses a jump table with address labels (&&LABEL) to dispatch directly to the
 * handler.
 */
#if PVM_USE_COMPUTED_GOTO

    /* The dispatch table contains the address of every label in handlers.inc */
    static const void *const dispatch_table[] = {
#include "handler_table.inc"
    };

/*
 * HANDLER macro defines the label target.
 * DISPATCH macro increments IP and jumps to the next handler.
 */
#define HANDLER(name) name
#define DISPATCH()                           \
    do                                       \
    {                                        \
        instr++;                             \
        goto *dispatch_table[instr->opcode]; \
    } while (0)

    /* Must perform the initial jump to start the race. */
    goto *dispatch_table[instr->opcode];

/* Include the instruction logic */
#include "handlers.inc"

#undef HANDLER
#undef DISPATCH

/*
 * Uses a standard switch statement. Slower due to bounds checking and lack of
 * branch prediction, but 100% portable and safe.
 */
#else

/*
 * HANDLER macro defines a switch case.
 * DISPATCH macro jumps back to the switch statement.
 */
#define HANDLER(name) case name
#define DISPATCH()    goto dispatch_loop

dispatch_loop:
    switch (instr->opcode)
    {
/* Include the instruction logic */
#include "handlers.inc"

        default:
            PVM_ASSERT_MSG(
                0, "Invalid Opcode in interpreter dispatch: %d", instr->opcode);
            break;
    }

#undef HANDLER
#undef DISPATCH
#endif
}
