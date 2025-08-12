// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include <cstring>
#include <cstdlib>

#include "Base/Logging/Log.h"

namespace aarch64
{
/* AArch64 R0-R31 */
#define GP_REGISTERS 32

/* AArch64 V0-V31 */
#define FP_REGISTERS 32

#define CACHE_LINE_SIZE 64
#define CPU_CORES 8

/*
 * vcpu_state_t - Holds the architectural state for an emulated vCPU.
 * @v:      The 128-bit vector registers V0-V31.
 * @r:      General purpose registers R0-R31.
 * @pc:     Program Counter.
 * @pstate: Process State Register (NZCV flags, EL, etc.).
 *
 * This structure is aligned to the L1 cache line size to prevent false
 * sharing when multiple host threads are emulating vCPUs on different
 * physical cores.
 */
typedef struct alignas(CACHE_LINE_SIZE)
{
    unsigned __int128 v[FP_REGISTERS];
    uint64_t r[GP_REGISTERS];
    uint64_t pc;
    uint32_t pstate;
} vcpu_state_t;
}  // namespace aarch64

//=========================================================
// OUTDATED CODE
//=========================================================
struct CPU
{
    u64 regs[31] = {0};  // X0–X30
    u64 pc = 0;
    static constexpr size_t MEM_SIZE = 64 * 1024;
    u8 memory[MEM_SIZE];

    CPU() { std::memset(memory, 0, MEM_SIZE); }

    u64& x(int i) { return regs[i]; }

    u8 read_byte(u64 addr)
    {
        if (addr >= MEM_SIZE)
        {
            LOG_INFO(ARM, "{} out of bounds", addr);
        }
        return memory[addr];
    }

    void write_byte(u64 addr, u8 byte)
    {
        if (addr >= MEM_SIZE)
        {
            LOG_INFO(ARM, "{} out of bounds", addr);
        }
        memory[addr] = byte;
    }

    void print_debug_information()
    {
        LOG_INFO(ARM, "PC = {}", pc);
        for (int reg = 0; reg < 31; reg++)
        {
            uint64_t regis = x(reg);
            LOG_INFO(ARM, "X{} = {}", reg, regis);  // X0 = 0..
        }
    }
};

void cpuTest();
