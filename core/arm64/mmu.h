#pragma once

#include "guest.h"
#include "isa.h"

namespace pound::arm64::memory
{
/*
 * mmu_gva_to_gpa() - Translate a Guest Virtual Address to a Guest Physical Address.
 * @vcpu:       A pointer to the vCPU state.
 * @memory:     A pointrr to the guest's memory.
 * @gva:        The Guest Virtual Address to translate.
 * @out_gpa:    A pointer to a uint64_t where the resulting Guest Physical Address
 *              will be stored on success.
 *
 * This function is the primary entry point for the emulated AArch64 Stage 1
 * MMU. It is responsible for resolving a virtual address used by the guest
 * into a physical address within the guest's physical address space.
 *
 * The translation behavior is dependent on the state of the emulated MMU,
 * primarily controlled by the SCTLR_EL1.M bit (MMU enable).
 *
 * If the MMU is disabled, this function performs an identity mapping, where
 * the GPA is identical to the GVA. This correctly models the processor's
 * behavior on reset and is the initial "stub" implementation.
 *
 * If the MMU is enabled, this function will perform a full, multi-level page
 * table walk, starting from the base address in TTBR0_EL1 or TTBR1_EL1. It
 * will parse translation table descriptors, check for permissions, and handle
 * different page sizes as configured in TCR_EL1.
 *
 * A failed translation will result in a fault. The caller is responsible for
 * checking the return value and initiating a synchronous exception if a fault
 * occurs. The contents of @out_gpa are undefined on failure.
 *
 * Return: 0 on successful translation. A negative error code on a translation
 * fault (e.g., for a page fault, permission error, or alignment fault).
 */
int mmu_gva_to_gpa(pound::arm64::vcpu_state_t* vcpu, guest_memory_t* memory, uint64_t gva, uint64_t* out_gpa);
}  // namespace pound::arm64::memory
