#include "kvm.h"
#include "guest.h"
#include "common/passert.h"
#include "host/memory/arena.h"

#define LOG_MODULE "kvm"
#include "common/logging.h"

namespace pound::kvm
{

uint8_t kvm_probe(kvm_t* kvm, enum target_type type)
{
    if (type != KVM_TARGET_SWITCH1)
    {
        PVM_ASSERT_MSG(false, "Only Switch 1 is supported");
    }
    kvm->ops = s1_ops;
    /* Go to targets/switch1/hardware/probe.cpp */
    (void)kvm->ops.init(kvm);
    return 0;
}

void take_synchronous_exception(kvm_vcpu_t* vcpu, uint8_t exception_class, uint32_t iss, uint64_t faulting_address)
{
    PVM_ASSERT(nullptr != vcpu);
    /* An EC holds 6 bits.*/
    PVM_ASSERT(0 == (exception_class & 11000000));
    /* An ISS holds 25 bits */
    PVM_ASSERT(0 == (iss & 0xFE000000));

    vcpu->elr_el1 = vcpu->pc;
    vcpu->spsr_el1 = vcpu->pstate;
    vcpu->esr_el1 = 0;

    /* Bits [31:26] are the Exception Class (EC). */
    /* Bits [25] is the Instruction Length (IL), 1 for a 32-bit instruction. */
    /* Bits [24:0] are the Instruction Specific Syndrome (ISS) */
    const uint64_t esr_il_bit = (1ULL << 25);
    vcpu->esr_el1 = ((uint64_t)exception_class << 26) | esr_il_bit | iss;

    if ((exception_class == EC_DATA_ABORT) || (exception_class == EC_DATA_ABORT_LOWER_EL))
    {
        vcpu->far_el1 = faulting_address;
    }

    /* The CPU state must be changed to a known safe state for handling */
    vcpu->pstate &= ~0xF0000000;

    /* Mask asynchronous exceptions (IRQ, FIQ, SError). We dont want the
     * Exception handler to be interruoted by a less important event. */
    const uint32_t PSTATE_IRQ_BIT = (1 << 7);
    const uint32_t PSTATE_FIQ_BIT = (1 << 6);
    const uint32_t PSTATE_SERROR_BIT = (1 << 8);
    vcpu->pstate |= (PSTATE_IRQ_BIT | PSTATE_FIQ_BIT | PSTATE_SERROR_BIT);

    /* Set the target exception level to EL1. The mode field M[3:0] is set
     * to 0b0101 for EL1h (using SP_EL1). (page 913 in manual) */
    const uint32_t PSTATE_EL_MASK = 0b1111;
    vcpu->pstate &= ~PSTATE_EL_MASK;
    vcpu->pstate |= PSTATE_EL1H;

    /*  TODO(GloriousTacoo:arm): DO NOT IMPLEMENT UNTIL THE INSTRUCTION
     *  DECODER IS FINISHED.
     *
     *  Create an Exception Vector Table, determine
     *  the address of the exception handler, then update the PC.
     *  
     *  vcpu->pc = vcpu->vbar_el1 + offset; */
}

void cpuTest()
{
#if 0
    pound::host::memory::arena_t guest_memory_arena = pound::host::memory::arena_init(GUEST_RAM_SIZE);
    PVM_ASSERT(nullptr != guest_memory_arena.data);

    memory::guest_memory_t* guest_ram = memory::guest_memory_create(&guest_memory_arena);

    //(void)test_guest_ram_access(guest_ram);
#endif
}  
}  // namespace pound::kvm
