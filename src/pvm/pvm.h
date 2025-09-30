// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once
#include "guest.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace pound::pvm
{
/* AArch64 R0-R31 */
#define GP_REGISTERS 32

#define CACHE_LINE_SIZE 64
#define GUEST_RAM_SIZE 10240  // 10KiB
#define CPU_CORES 8

/* Data Abort exception taken without a change in Exception level. */
#define EC_DATA_ABORT 0b100101

/* Data Abort exception from a lower Exception level. */
#define EC_DATA_ABORT_LOWER_EL 0b100100

/* Set the PSTATE exception level. (page 913 in manual) */
#define PSTATE_EL0 0b0000
#define PSTATE_EL1T 0b0100
#define PSTATE_EL1H 0b0101

/*
 * pvm_vcpu_t - Holds the architectural and selected system-register state for an emulated vCPU.
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
 * @sctlr_el1:      System Control Register.
 * @spsr_el1:       Saved Program Status Register.
 * @tcr_el1:        Translation Control Register.
 * @ttbr0_el1:      Translation Table Base Register 0.
 * @ttbr1_el1:      Translation Table Base Register 1.
 * @vbar_el1:       Vector Base Address Register.
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

    /* Bit [0] bit enables the MMU. */
    uint64_t sctlr_el1;

    /*
     * A snapshot of the current PSTATE register before the exception. 
     * This is for restoring the program's state when returning from an
     * exception. 
     */
    uint64_t spsr_el1;

    /* Bits [5:0], T0SZ, specifies the size of the bottom half of the
     * virtual address space (the ones controlled by TTBR0).
     *
     * Bits [21:16], T1SZ, does the same for the top half of the virtual
     * address space (controlled by TTBR1). */
    uint64_t tcr_el1;

    /*
     * Holds the 64-bit base physical address of the initial page table
     * used for translating virtual addresses in the lower half of the
     * virtual address space (typically userspace). The top bit of the VA
     * (bit 63) being 0 selects TTBR0 for the page table walk.
     */
    uint64_t ttbr0_el1;

    /*
     * Holds the 64-bit base physical address of the initial page table
     * used for translating virtual addresses in the upper half of the
     * virtual address space (typically kernel space). The top bit of the VA
     * (bit 63) being 1 selects TTBR1 for the page table walk.
     */
    uint64_t ttbr1_el1;

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
} pvm_vcpu_t;

/* This is here to break the circular dependency between pvm_t and pvm_ops_t. */
typedef struct pvm_s pvm_t;

/*
 * struct pvm_ops_t - A table of machine-specific operations.
 * @init:       Function pointer to initialize the target machine's state.
 *              Called once by pvm_probe(). It is responsible for setting up
 *              the guest memory map, loading firmware, and registering all
 *              MMIO device handlers.
 * @destroy:    Function pointer to clean up and deallocate all resources
 *              associated with the machine instance. Called on VM shutdown.
 *
 * This structure acts as a "virtual table" in C, allowing the generic pvm
 * core to call target-specific code (e.g., for a Switch 1 or Switch 2)
 * without needing to know the implementation details. Each supported target
 * machine must provide a globally visible instance of this struct.
 */
typedef struct
{
    /* Initialize the machine state */
    int8_t (*init)(pvm_t* pvm);

    /* Clean up on shutdown */
    void (*destroy)(pvm_t* pvm);
} pvm_ops_t;

/*
 * pvm_s - The main pvm instance structure.
 * @vcpu:   The state of the emulated virtual CPU core. Contains all guest-
 *          visible registers.
 * @memory: A structure representing the guest's physical RAM.
 * @ops:    A table of function pointers to the machine-specific implementation
 *          for this pvm instance. This should only be set by pvm_probe().
 *
 * This structure represents a single virtual machine instance.
 */
struct pvm_s
{
    pound::pvm::pvm_vcpu_t vcpu;
    pound::pvm::memory::guest_memory_t memory;
    pvm_ops_t ops;
};

/**
 * s1_ops - The machine-specific operations for the "Switch 1" target.
 *
 * This is the global instance of the operations table for the Switch 1.
 * It is defined in the target-specific source file 
 * (targets/switch1/hardware/probe.cpp) and provides the iplementations 
 * for initializing and running the emulated Switch 1 hardware.
 */
extern const pvm_ops_t s1_ops;

enum target_type
{
    pvm_TARGET_SWITCH1 = 0,
    pvm_TARGET_SWITCH2 = 1,
};

/*
 * pvm_probe - Probes for and initializes a target machine configuration.
 * @pvm:  A pointer to the pvm instance to be initialized. This function will
 *        populate the fields of this struct.
 * @type: The type of target machine to initialize.
 *
 * This function is the primary factory for creating a virtual machine. It
 * looks up the requested machine @type, attaches the corresponding operations
 * table (e.g., s1_ops) to the @pvm instance, and calls the machine-specific
 * init() function.
 *
 * On successful return, the @pvm struct will be fully configured and ready
 * for execution.
 *
 * Return: 0 on success, or a negative errno code on failure.
 */
uint8_t pvm_probe(pvm_t* pvm, enum target_type type);

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
void take_synchronous_exception(pvm_vcpu_t* vcpu, uint8_t exception_class, uint32_t iss, uint64_t faulting_address);

void cpuTest();
}  // namespace pound::pvm
