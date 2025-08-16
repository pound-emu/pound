// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include <cstdlib>
#include <cstring>

#include "Base/Logging/Log.h"

namespace pound::arm64
{
/* AArch64 R0-R31 */
#define GP_REGISTERS 32

/* AArch64 V0-V31 */
#define FP_REGISTERS 32

#define CACHE_LINE_SIZE 64
#define GUEST_RAM_SIZE 10240  // 10KiB
#define CPU_CORES 8

/* Data Abort exception taken without a change in Exception level. */
#define EC_DATA_ABORT 0b100101

/* Data Abort exception from a lower Exception level. */
#define EC_DATA_ABORT_LOWER_EL 0b100100

/*
 * vcpu_state_t - Holds the architectural and selected system-register state for an emulated vCPU.
 * @v:              128-bit SIMD/FP vector registers V0–V31.
 * @r:              General-purpose registers X0–X31 (X31 as SP/ZR as appropriate).
 * @pc:             Program Counter.
 * @cntfreq_el0:    Counter Frequency.
 * @cntpct_el0:     Physical Counter.
 * @cntvct_el0:     Virtual Counter - CRITICAL for timing.
 * @cntv_cval_el0:  Virtual Timer Compare Value.
 * @pmccntr_el0:    Cycle Counter.
 * @tpidr_el0:      Thread Pointer ID Register.
 * @tpidrro_el0:    Thread Pointer ID, read-only.
 * @elr_el1:        Exception Link Register.
 * @esr_el1:        Exception Syndrome Register.
 * @far_el1:        Fault Address Register.
 * @vbar_el1:       Vector Base Address Register.
 * @spsr_el1:       Saved Program Status Register.
 * @ctr_el0:        Cache-Type.
 * @cntv_ctl_el0:   Virtual Timer Control.
 * @dczid_el0:      Data Cache Zero ID.
 * @pmcr_el0:       Performance Monitor Counter.
 * @pstate:         Process State Register (NZCV, DAIF, EL, etc.).
 *
 * This structure is aligned to the L1 cache line size to prevent false sharing
 * when multiple host threads are emulating vCPUs on different physical cores.
 */
typedef struct alignas(CACHE_LINE_SIZE)
{
    unsigned __int128 v[FP_REGISTERS];
    uint64_t r[GP_REGISTERS];
    uint64_t pc;
    uint64_t cntfreq_el0;
    uint64_t cntpct_el0;
    uint64_t cntvct_el0;
    uint64_t cntv_cval_el0;
    uint64_t pmccntr_el0;
    uint64_t tpidr_el0;
    uint64_t tpidrro_el0;

    /*
     * Stores the Program Counter of the instruction that was interrupted.
     * For a synchronous fault, it's the address of the faulting instruction
     * itself.
     */
    uint64_t elr_el1;

    /*
     * Tells the guest OS why the exception happened. It contains a high
     * level Exception Class (EC) (eg, Data Abort) and a low level 
     * Instruction Specific Syndrome (ISS) with fine-grained details.
     * (eg, it was an allignment fault cause by a write operation.)
     */
    uint64_t esr_el1;

    /* The memory address that caused a Data Abort exception. */
    uint64_t far_el1;

    /*
     * A snapshot of the current PSTATE register before the exception. 
     * This is for restoring the program's state when returning from an
     * exception. 
     */
    uint64_t spsr_el1;

    /* 
     * The base address in guest memory where the Exception Vector Table
     * can be found.
     */
    uint64_t vbar_el1;

    uint32_t ctr_el0;
    uint32_t cntv_ctl_el0;
    uint32_t dczid_el0;
    uint32_t pmcr_el0;
    uint32_t pstate;
} vcpu_state_t;

/*
 * take_synchronous_exception() -  Emulates the hardware process of taking a synchronous exception to EL1.
 *
 * @vcpu:               A pointer to the vCPU state to be modified.
 * @exception_class:    The high-level Exception Class (EC) code for ESR_EL1.
 * @iss:                The low-level Instruction Specific Syndrome (ISS) code for ESR_EL1.
 * @faulting_address:   The faulting address, to be written to FAR_EL1. Only valid for Data/Instruction Aborts. Pass 0 for other exception types.
 *
 * This function modifies the vCPU state according to the rules for taking a
 * synchronous exception from a lower or same exception level that is targeting EL1.
 * It saves the necessary return state, populates the syndrome registers,
 * updates the processor state for entry into EL1, and calculates the new
 * program counter by branching to the appropriate offset in the EL1 vector table.
 *
 */
void take_synchronous_exception(vcpu_state_t* vcpu, uint8_t exception_class, uint32_t iss, uint64_t faulting_address);

void cpuTest();
}  // namespace pound::arm64
