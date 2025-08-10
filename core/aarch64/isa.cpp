#include "isa.h"
#include "Base/Assert.h"

void cpuTest()
{
    aarch64::vcpu_state_t vcpu_states[CPU_CORES] = {};

    // Outdated Code
    CPU cpu;
    cpu.pc = 0;

    // Simple ARMv8 program in memory (MOVZ X0, #5; ADD X0, X0, #3; RET)
    // These are placeholders; real encoding will be parsed later
    cpu.write_byte(0, 0x05);  // MOVZ placeholder
    cpu.write_byte(4, 0x03);  // ADD placeholder
    cpu.write_byte(8, 0xFF);  // RET placeholder

    LOG_INFO(ARM, "{}", cpu.read_byte(0));
    cpu.print_debug_information();
}
