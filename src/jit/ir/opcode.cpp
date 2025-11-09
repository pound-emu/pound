#include "opcode.h"
#include "type.h"
#include <stddef.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::ir {
#define OPCODE_ARGS_TYPES_SIZE 4
#define OPCODE_ARRAY_SIZE      386

typedef struct
{
    const char *name;
    type_t      type;
    type_t      arg_types[OPCODE_ARGS_TYPES_SIZE];
} decoded_opcode_t;

decoded_opcode_t opcodes[OPCODE_ARRAY_SIZE] = {
#define OPCODE(name, type, ...) \
    decoded_opcode_t { #name, type, { __VA_ARGS__ } },
#define A32OPC(name, type, ...) \
    decoded_opcode_t { #name, type, { __VA_ARGS__ } },
// #define A64OPC(name, type, ...) decoded_opcode_t{#name, type, {__VA_ARGS__}},
#include "./opcode.inc"
#undef OPCODE
    // #undef A32OPC
    // #undef A64OPC
};

void
opcode_init (void)
{
    for (size_t i = 0; i < OPCODE_ARRAY_SIZE; ++i)
    {
        LOG_TRACE("Opcode Registered: %s", opcodes[i].name);
    }
}
} // namespace pound::jit::ir
