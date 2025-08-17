#include "mmu.h"

namespace pound::arm64::memory
{
int mmu_gva_to_gpa(pound::arm64::vcpu_state_t* vcpu, uint64_t gva, uint64_t* out_gpa)
{
    const uint8_t SCTLR_EL1_M_BIT = (1 << 0);
    if (0 == (vcpu->sctlr_el1 & SCTLR_EL1_M_BIT))
    {
        *out_gpa = gva;
        return 0;
    }

    return -1;
}
}  // namespace pound::arm64::memory
