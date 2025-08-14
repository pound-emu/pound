// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include <cstdlib>
#include <cstring>

#include "Base/Logging/Log.h"

namespace aarch64
{
/* AArch64 R0-R31 */
#define GP_REGISTERS 32

/* AArch64 V0-V31 */
#define FP_REGISTERS 32

#define CACHE_LINE_SIZE 64
#define GUEST_RAM_SIZE 10240  // 10KiB
#define CPU_CORES 8

/*
 * vcpu_state_t - Holds the architectural and selected system-register state for an emulated vCPU.
 * @v:      128-bit SIMD/FP vector registers V0–V31.
 * @r:      General-purpose registers X0–X31 (X31 as SP/ZR as appropriate).
 * @pc:     Program Counter.
 * @pstate: Process State Register (NZCV, DAIF, EL, etc.).
 *
 * System registers (subset mirrored for fast-path emulation at EL0):
 * - ctr_el0, dczid_el0: Cache/type identification.
 * - tpidrro_el0, tpidr_el0: Thread pointers (host-mapped TLS pointers).
 * - cntfrq_el0, cntpct_el0, cntvct_el0, cntv_ctl_el0, cntv_cval_el0: Generic timers/counters.
 * - pmccntr_el0, pmcr_el0: PMU cycle counter and control.
 *
 * This structure is aligned to the L1 cache line size to prevent false sharing
 * when multiple host threads are emulating vCPUs on different physical cores.
 */
typedef struct alignas(CACHE_LINE_SIZE)
{
    unsigned __int128 v[FP_REGISTERS];
    uint64_t r[GP_REGISTERS];
    uint64_t pc;
    uint32_t pstate;

    // ========================= System Registers ==================================
    // Basics
    uint32_t ctr_elo;               // cache-type register 
    uint32_t dczid_elo;             // data cache zero-ID
    const uint64_t* tpidrro_e10;    // thread pointer ID register, read-only
    uint64_t* tpidr_e10;            // thread pointer ID register
    
    // Counters
    uint64_t cntfreq_elo;           // counter frequency
    uint64_t cntvct_el0;     // Virtual counter - CRITICAL for timing
    uint64_t cntpct_el0;     // Physical counter
    uint32_t cntv_ctl_el0;   // Virtual timer control
    uint64_t cntv_cval_el0;  // Virtual timer compare value

    // Performance monitoring (if games use them):
    uint64_t pmccntr_el0;    // Cycle counter
    uint32_t pmcr_el0;       // Performance monitor control
    // =============================================================================
} vcpu_state_t;

namespace memory
{
/*
 * guest_memory_t - Describes a contiguous block of guest physical RAM.
 * @base: Pointer to the start of the host-allocated memory block.
 * @size: The size of the memory block in bytes.
 */
typedef struct
{
    uint8_t* base;
    uint64_t size;
} guest_memory_t;

/*
 * gpa_to_hva() - Translate a Guest Physical Address to a Host Virtual Address.
 * @memory: The guest memory region to translate within.
 * @gpa: The Guest Physical Address (offset) to translate.
 *
 * This function provides a fast, direct translation for a flat guest memory
 * model. It relies on the critical pre-condition that the guest's physical
 * RAM is backed by a single, contiguous block of virtual memory in the host's
 * userspace (typically allocated with mmap()).
 *
 * In this model, memory->base is the Host Virtual Address (HVA) of the start of
 * the backing host memory. The provided Guest Physical Address (gpa) is not
 * treated as a pointer, but as a simple byte offset from the start of the guest's
 * physical address space (PAS).
 *
 * The translatiom is therefore a single pointer-offset calculation. This establishes
 * a direct 1:1 mapping between the guest's PAS and the host's virtual memory block.
 *
 * The function asserts that GPA is within bounds. The caller is responsible for
 * ensuring the validity of the GPA prior to calling.
 *
 * Return: A valid host virtual address pointer corresponding to the GPA.
 */
static inline uint8_t* gpa_to_hva(guest_memory_t* memory, uint64_t gpa);
}  // namespace memory
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
