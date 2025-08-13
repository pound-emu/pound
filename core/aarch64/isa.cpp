#include "isa.h"
#include "Base/Assert.h"
#include "memory/arena.h"

static inline void* aarch64::memory::gpa_to_hva(aarch64::memory::guest_memory_t* memory, uint64_t gpa)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT(gpa < memory->size);
    void* hva = memory->base + gpa;
    return hva;
}

void cpuTest()
{
    aarch64::vcpu_state_t vcpu_states[CPU_CORES] = {};
    memory::arena_t guest_memory_arena = memory::arena_init(GUEST_RAM_SIZE);
    ASSERT(nullptr != guest_memory_arena.data);

    aarch64::memory::guest_memory_t guest_ram = {};
    guest_ram.base = guest_memory_arena.data;
    guest_ram.size = guest_memory_arena.capacity;

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
