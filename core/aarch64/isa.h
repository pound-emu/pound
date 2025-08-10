// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include <cstring>

#include "Base/Logging/Log.h"

namespace aarch64
{
#define GPR_REGISTERS 32
#define ZERO_REGISTER_INDEX 31

#define FPR_REGISTERS 32

typedef struct
{
    uint64_t gpr[GPR_REGISTERS];
    unsigned __int128 fpr[FPR_REGISTERS];
    uint64_t pc;
    uint64_t sp;
} isa_t;

uint64_t read_X(uint64_t* registers, size_t n);
void adr(uint64_t* registers, size_t n, uint64_t pc, uint64_t offset);
//=========================================================
// Access Floating Point Registers
//=========================================================

uint8_t B(unsigned __int128 registers, size_t n);
uint16_t H(unsigned __int128 registers, size_t n);
uint32_t S(unsigned __int128 registers, size_t n);
uint64_t D(unsigned __int128 registers, size_t n);
unsigned __int128 Q(unsigned __int128 registers, size_t n);

}  // namespace aarch64

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
