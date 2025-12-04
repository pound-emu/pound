#include "opcode.h"
#include <stddef.h>

#define LOG_MODULE "jit"
#include "common/logging.h"

pvm_jit_ir_decoded_opcode_t g_pvm_jit_ir_opcodes[NUM_OPCODE] = {
#define OPCODE(name, type, ...) { #name, type, { __VA_ARGS__ } },
#define A32OPC(name, type, ...) { #name, type, { __VA_ARGS__ } },
// #define A64OPC(name, type, ...) pvm_jit_ir_decoded_opcode_t{#name, type,
// {__VA_ARGS__}},
#include "./opcode.inc"
#undef OPCODE
#undef A32OPC
    // #undef A64OPC
};
