#pragma once 

namespace pound::armv8
/**
 * kvm_mmu_gva_to_gpa() - Translate a Guest Virtual Address to a Guest Physical Address.
 * @vcpu:   The vCPU state, containing MMU configuration (TTBR0_EL1, etc.).
 * @gva:    The Guest Virtual Address to translate.
 * @gpa:    A pointer to store the resulting Guest Physical Address.
 *
 * This function is the entry point for the emulated MMU.
 *
 * For now, this is a stub that implements identity mapping (GVA -> GPA)
 * when the MMU is disabled, which is the correct architectural behavior
 * on reset.
 *
 * Return: 0 on success, or a negative error code on a fault.
 */
int kvm_mmu_gva_to_gpa(vcpu_state_t* vcpu, uint64_t gva, uint64_t* gpa);
}
