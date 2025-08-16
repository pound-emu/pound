#include "isa.h"
#include "Base/Assert.h"
#include "memory.h"
#include "memory/arena.h"

namespace pound::aarch64
{
void take_synchronous_exception(vcpu_state_t* vcpu, uint8_t exception_class, uint32_t iss, uint64_t faulting_address)
{
    ASSERT(nullptr != vcpu);
    /* An EC holds 6 bits.*/
    ASSERT(0 == (exception_class & 11000000));
    /* An ISS holds 25 bits */
    ASSERT(0 == (iss & 0xFE000000));

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
    const uint32_t PSTATE_EL1H = 0b0101;
    vcpu->pstate |= PSTATE_EL1H;

    /*  TODO(GloriousTacoo:arm): DO NOT IMPLEMENT UNTIL THE INSTRUCTION
     *  DECODER IS FINISHED.
     *
     *  Create an Exception Vector Table, determine
     *  the address of the exception handler, then update the PC.
     *  
     *  vcpu->pc = vcpu->vbar_el1 + offset; */
}

/** THIS FUNCTION WAS MADE WITH AI AND IS CALLED WHEN RUNNING THE CPU TEST FROM THE GUI!
 *
 * @brief Runs a comprehensive suite of tests on the guest memory access functions using the project's logging system.
 *
 * This function systematically tests the read and write capabilities of the memory
 * subsystem for all standard data sizes (8, 16, 32, and 64 bits). It verifies
 * that data written to memory can be correctly read back.
 *
 * It specifically checks:
 * 1. A standard aligned address in the middle of RAM.
 *
 * @param memory A pointer to an initialized guest_memory_t struct.
 * @return true if all tests pass, false otherwise.
 */
bool test_guest_ram_access(pound::aarch64::memory::guest_memory_t* memory)
{
    LOG_INFO(Memory, "--- [ Starting Guest RAM Access Test ] ---");
    if (memory == nullptr || memory->base == nullptr || memory->size < 4096)
    {
        LOG_CRITICAL(Memory, "Invalid memory block provided. Cannot run tests.");
        return false;
    }

    bool all_tests_passed = true;

#define RUN_TEST(description, condition)                                                       \
    do                                                                                         \
    {                                                                                          \
        char log_buffer[256];                                                                  \
        if (condition)                                                                         \
        {                                                                                      \
            snprintf(log_buffer, sizeof(log_buffer), "  [TEST] %-45s... [PASS]", description); \
            LOG_INFO(Memory, log_buffer);                                                      \
        }                                                                                      \
        else                                                                                   \
        {                                                                                      \
            snprintf(log_buffer, sizeof(log_buffer), "  [TEST] %-45s... [FAIL]", description); \
            LOG_ERROR(Memory, log_buffer);                                                     \
            all_tests_passed = false;                                                          \
        }                                                                                      \
    } while (0)

#define VERIFY_ACCESS(size, suffix, addr, write_val)                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        guest_mem_write##suffix(memory, addr, write_val);                                                              \
        uint##size##_t read_val = guest_mem_read##suffix(memory, addr);                                                \
        bool success = (read_val == write_val);                                                                        \
        RUN_TEST("Write/Read " #size "-bit", success);                                                                 \
        if (!success)                                                                                                  \
        {                                                                                                              \
            char error_buffer[256];                                                                                    \
            snprintf(error_buffer, sizeof(error_buffer), "    -> At GPA 0x%016llx, Expected 0x%016llx, Got 0x%016llx", \
                     (unsigned long long)addr, (unsigned long long)write_val, (unsigned long long)read_val);           \
            LOG_ERROR(Memory, error_buffer);                                                                           \
        }                                                                                                              \
    } while (0)

    // --- 1. Test a typical, aligned address in the middle of memory ---
    LOG_INFO(Memory, "[INFO] Testing standard access at a midrange address (GPA 0x1000)...");
    uint64_t test_addr = 0x1000;
    VERIFY_ACCESS(8, b, test_addr + 0, 0xA5);
    VERIFY_ACCESS(16, w, test_addr + 2, 0xBEEF);
    VERIFY_ACCESS(32, l, test_addr + 4, 0xDEADBEEF);
    VERIFY_ACCESS(64, q, test_addr + 8, 0xCAFEBABE01234567);

    // --- 2. Test the very beginning of the memory block ---
    LOG_INFO(Memory, "[INFO] Testing boundary access at the start of RAM (GPA 0x0)...");
    VERIFY_ACCESS(64, q, 0x0, 0xFEEDFACEDEADBEEF);

    // --- 3. Test the very end of the memory block ---
    LOG_INFO(Memory, "[INFO] Testing boundary access at the end of RAM...");

    uint64_t end_addr_b = memory->size - 1;
    uint64_t end_addr_w = (memory->size - 2) & ~1ULL;
    uint64_t end_addr_l = (memory->size - 4) & ~3ULL;
    uint64_t end_addr_q = (memory->size - 8) & ~7ULL;

    VERIFY_ACCESS(8, b, end_addr_b, 0xFE);
    VERIFY_ACCESS(16, w, end_addr_w, 0xFEFE);
    VERIFY_ACCESS(32, l, end_addr_l, 0xFEFEFEFE);
    VERIFY_ACCESS(64, q, end_addr_q, 0xFEFEFEFEFEFEFEFE);

    // --- 4. Final Verdict ---
    LOG_INFO(Memory, "--- [ Guest RAM Access Test Finished ] ---");
    if (all_tests_passed)
    {
        LOG_INFO(Memory, ">>> Result: ALL TESTS PASSED");
    }
    else
    {
        LOG_ERROR(Memory, ">>> Result: SOME TESTS FAILED");
    }
    LOG_INFO(Memory, "----------------------------------------------");

    return all_tests_passed;
}

void cpuTest()
{
    vcpu_state_t vcpu_states[CPU_CORES] = {};
    pound::memory::arena_t guest_memory_arena = pound::memory::arena_init(GUEST_RAM_SIZE);
    ASSERT(nullptr != guest_memory_arena.data);

    pound::aarch64::memory::guest_memory_t guest_ram = {};
    guest_ram.base = static_cast<uint8_t*>(guest_memory_arena.data);
    guest_ram.size = guest_memory_arena.capacity;

    (void)test_guest_ram_access(&guest_ram);
}
}  // namespace pound::aarch64
