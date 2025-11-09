#include "opcode.h"
#include <stddef.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

namespace pound::jit::ir {

decoded_opcode_t g_opcodes[NUM_OPCODE] = {
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
    for (size_t i = 0; i < NUM_OPCODE; ++i)
    {
        LOG_TRACE("Opcode Registered: %s", g_opcodes[i].name);
    }
}
} // namespace pound::jit::ir
