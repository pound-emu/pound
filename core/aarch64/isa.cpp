#include "isa.h"
#include "Base/Assert.h"
#include "memory/arena.h"

// TODO(GloriousTacoo:aarch64) Implement big to little endian conversion for guest_mem read and write functions.

namespace pound::aarch64
{
static inline uint8_t* gpa_to_hva(guest_memory_t* memory, uint64_t gpa)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT(gpa < memory->size);
    uint8_t* hva = memory->base + gpa;
    return hva;
}

/*
 * ============================================================================
 *                          Guest Memory Read Functions
 * ============================================================================
 */

static inline uint8_t guest_mem_readb(guest_memory_t* memory, uint64_t gpa)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT(gpa <= memory->size);
    uint8_t* hva = gpa_to_hva(memory, gpa);
    return *hva;
}

static inline uint16_t guest_mem_readw(guest_memory_t* memory, uint64_t gpa)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT((gpa + sizeof(uint16_t)) <= memory->size);
    // Check if gpa is aligned to 2 bytes.
    ASSERT((gpa & 1) == 0);
    uint16_t* hva = (uint16_t*)gpa_to_hva(memory, gpa);
    return *hva;
}

static inline uint32_t guest_mem_readl(guest_memory_t* memory, uint64_t gpa)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT((gpa + sizeof(uint32_t)) <= memory->size);
    // Check if gpa is aligned to 4 bytes.
    ASSERT((gpa & 3) == 0);
    uint32_t* hva = (uint32_t*)gpa_to_hva(memory, gpa);
    return *hva;
}

static inline uint64_t guest_mem_readq(guest_memory_t* memory, uint64_t gpa)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT((gpa + sizeof(uint64_t)) <= memory->size);
    // Check if gpa is aligned to 8 bytes.
    ASSERT((gpa & 7) == 0);
    uint64_t* hva = (uint64_t*)gpa_to_hva(memory, gpa);
    return *hva;
}

/*
 * ============================================================================
 *                          Guest Memory Write Functions
 * ============================================================================
 */

static inline void guest_mem_writeb(guest_memory_t* memory, uint64_t gpa, uint8_t val)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT(gpa <= memory->size);
    uint8_t* hva = gpa_to_hva(memory, gpa);
    *hva = val;
}

static inline void guest_mem_writew(guest_memory_t* memory, uint64_t gpa, uint16_t val)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT((gpa + sizeof(uint16_t)) <= memory->size);
    // Check if gpa is aligned to 2 bytes.
    ASSERT((gpa & 1) == 0);
    uint16_t* hva = (uint16_t*)gpa_to_hva(memory, gpa);
    *hva = val;
}

static inline void guest_mem_writel(guest_memory_t* memory, uint64_t gpa, uint32_t val)
{
    ASSERT(nullptr != memory->base);
    ASSERT((gpa + sizeof(uint32_t)) <= memory->size);
    // Check if gpa is aligned to 4 bytes.
    ASSERT((gpa & 3) == 0);
    uint32_t* hva = (uint32_t*)gpa_to_hva(memory, gpa);
    *hva = val;
}

static inline void guest_mem_writeq(guest_memory_t* memory, uint64_t gpa, uint64_t val)
{
    ASSERT(nullptr != memory);
    ASSERT(nullptr != memory->base);
    ASSERT((gpa + sizeof(uint64_t)) <= memory->size);
    // Check if gpa is aligned to 8 bytes.
    ASSERT((gpa & 7) == 0);
    uint64_t* hva = (uint64_t*)gpa_to_hva(memory, gpa);
    *hva = val;
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
bool test_guest_ram_access(guest_memory_t* memory)
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
    memory::arena_t guest_memory_arena = memory::arena_init(GUEST_RAM_SIZE);
    ASSERT(nullptr != guest_memory_arena.data);

    guest_memory_t guest_ram = {};
    guest_ram.base = static_cast<uint8_t*>(guest_memory_arena.data);
    guest_ram.size = guest_memory_arena.capacity;

    (void)test_guest_ram_access(&guest_ram);
}
}  // namespace pound::aarch64
