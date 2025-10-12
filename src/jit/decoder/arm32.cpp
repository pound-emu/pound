#include "arm32.h"
namespace pound::jit::decoder
{
arm32_decoder_t g_arm32_decoder = {};

void arm32_add_instruction(arm32_decoder_t* decoder, const char* nane, arm32_opcode_t mask, arm32_opcode_t expected,
                           arm32_handler_fn handler)
{
}

void arm32_ADD_imm_handler(arm32_decoder_t* decoder, arm32_instruction_t instruction) {}

void arm32_init(arm32_decoder_t* decoder)
{
#define INST(fn, name, bitstring) arm32_add_instruction(decoder, name, 0, 0, &arm32_##fn##_handler);
#include "./arm32.inc"
#undef INST
}
}  // namespace pound::jit::decoder
