#include "mmu.h"
#include <limits.h>
#include "kvm.h"

namespace pound::kvm::memory
{
#define GRANULE_4KB (1ULL << 12)
#define GRANULE_16KB (1ULL << 14)
#define GRANULE_64KB (1ULL << 16)

/*
 * COUNT_TRAILING_ZEROS - Get the number of trailing zero bits in a u64
 * @x: A 64-bit value, which must be non-zero.
 *
 * Provides a portable wrapper around compiler-specific intrinsics for the
 * "Count Trailing Zeros" operation. This is equivalent to finding the bit
 * index of the least significant bit (LSB).
 *
 * Note: The behavior for an input of zero is undefined for __builtin_ctzll.
 * Callers must ensure the argument is non-zero. The MSVC wrapper handles
 * this by returning 64, but we should not rely on this behavior.
 */
#if defined(__GNUC__) || defined(__clang__)
#define COUNT_TRAILING_ZEROS(x) (uint8_t)__builtin_ctzll(x)
#elif defined(_MSC_VER)
#include <intrin.h>
/* MSVC's intrinsic is a bit more complex to use safely */
static inline uint8_t msvc_ctzll(unsigned long long val)
{
    unsigned long index = 0;
    if (_BitScanForward64(&index, val))
    {
        return (uint8_t)index;
    }
    return 64;
}
#define COUNT_TRAILING_ZEROS(x) msvc_ctzll(x)
#else
#error "Compiler not supported for CTZ intrinsic. Please add a fallback."
#endif

/* Define the size of a page table entry (descriptor) */
#define PAGE_TABLE_ENTRY_SHIFT 3 /* log2(8 bytes) */

int mmu_gva_to_gpa(pound::kvm::kvm_vcpu_t* vcpu, guest_memory_t* memory, uint64_t gva, uint64_t* out_gpa)
{
    const uint8_t SCTLR_EL1_M_BIT = (1 << 0);
    if (0 == (vcpu->sctlr_el1 & SCTLR_EL1_M_BIT))
    {
        *out_gpa = gva;
        return 0;
    }

    /* Extract T0SZ (bits [5:0]) and T1SZ (bits [21:16]) from TCR_EL1.
     * Both are 6-bit fields. */
    const uint64_t TxSZ_WIDTH = 6;
    const uint64_t TxSZ_MASK = (1ULL << TxSZ_WIDTH) - 1;

    const uint8_t T0SZ = vcpu->tcr_el1 & TxSZ_MASK;
    const uint8_t T1SZ = (vcpu->tcr_el1 >> 16) & TxSZ_MASK;

    /* The virtual address size in bits. */
    uint8_t virtual_address_size = 0;

    bool is_ttbr0 = false;
    bool is_ttbr1 = false;

    /* 
     * Before starting a page table walk, the hardware must perform two checks:
     * 1. Classify the GVA as belonging to the lower half (user, TTBR0) or
     *    upper half (kernel, TTBR1) of the virtual address space.
     * 2. Validate that the GVA is correct for the configured VA size.
     *
     * The size of the VA space is configured by the TxSZ fields in TCR_EL1.
     * A TxSZ value of N implies a (64 - N)-bit address space. For any valid
     * address in this space, the top N bits must be a sign-extension of
     * bit (63 - N).
     *
     * For example, in a 48-bit space (TxSZ=16), bit 47 is the top bit.
     *  - For a lower-half address, bits [63:47] must all be 0.
     *  - For an upper-half address, bits [63:47] must all be 1.
     *
     * This sign-extension rule means that bit 63 will always have the same
    , * value as bit (63 - N) for any valid address. We can therefore use a
     * simple check of bit 63 as an efficient shortcut to classify the
     * address. The full canonical check that follows will then catch any
     * invalid (non-sign-extended) addresses. 
     *
     * Example Scenario:
     *
     * Kernel sets TCR_EL1.T0SZ = 16. This means it's using a 48-bit VA
     * space (64 - 16 = 48). The top 16 bits of any valid user-space
     * GVA must be 0.
     *
     * A GVA of 0x0001_0000_0000_0000 comes in.
     *
     * The top 16 bits are not all zero. An address translation fault is
     * generated and the page table walk is aborted.
     */
    if ((gva << 63) & 1)
    {
        /* Address appears to be in the Upper (Kernal) Half */

        virtual_address_size = 64 - T1SZ;
        const uint64_t top_bits_mask = (~0ULL << virtual_address_size);
        const uint64_t gva_tag = gva & top_bits_mask;
        const uint64_t ttbr1_tag = vcpu->ttbr1_el1 & top_bits_mask;

        if (gva_tag != ttbr1_tag)
        {
            /* TODO(GloriousTacoo:memory): Generate address translation fault */
            return -1;
        }
        is_ttbr1 = true;
    }
    else
    {
        /* Address appears to be in the Lower (User) Half */

        virtual_address_size = 64 - T0SZ;
        const uint64_t top_bits_mask = (~0ULL << virtual_address_size);
        if (0 != (gva & top_bits_mask))
        {
            /* TODO(GloriousTacoo:memory): Generate address translation fault */
            return -1;
        }
        is_ttbr0 = true;
    }

    /*
     * The preceding logic determined which address space (and thus
     * which TTBR) we're dealing with. Now we get the page size
     * in bytes from the correct TGx field. 
     */
    uint64_t granule_size = 0;
    assert((true == is_ttbr0) || (true == is_ttbr1));
    if (true == is_ttbr0)
    {
        /*
         * We're in userspace. We need to decode TCR_EL1.TG0, which is
         * at bits [15:14].
         *
         * Encoding for TG0:
         * 0b00: 4KB granule
         * 0b01: 64KB granule
         * 0b10: 16KB granule
         * 0b11: Reserved, will cause a fault.
         */
        const uint8_t TG0_SHIFT = 14;
        const uint8_t TG0_MASK = 0b11;
        const uint8_t TG0 = (vcpu->tcr_el1 >> TG0_SHIFT) & TG0_MASK;
        switch (TG0)
        {
            case 0b00:
                granule_size = GRANULE_4KB;
                break;
            case 0b01:
                granule_size = GRANULE_64KB;
                break;
            case 0b10:
                granule_size = GRANULE_16KB;
                break;
            default:
                /*
                 * This is an illegal configuration. The hardware will fault.
                 * For now, an assert will catch bad guest OS behaviour.
                 */
                assert(!"Invalid TG0 value in TCR_EL1");
        }
    }
    else
    {
        /* 
         * We're in kernel space. We decode TCR_EL1.TG1, which is at
         * bits [31:30]. Note that the encoding values are different
         * from TG0. Don't get caught out.
         *
         * Encoding for TG1:
         * 0b01: 16KB granule
         * 0b01: 4KB granule
         * 0b11: 64KB granule
         * 0b00: Reserved, will cause a fault.
         */
        const uint8_t TG1_SHIFT = 30;
        const uint8_t TG1_MASK = 0b11;
        const uint8_t TG1 = (vcpu->tcr_el1 >> TG1_SHIFT) & TG1_MASK;
        switch (TG1)
        {
            case 0b01:
                /* 16KB page size */
                granule_size = GRANULE_16KB;
                break;
            case 0b10:
                /* 4KB page size */
                granule_size = GRANULE_4KB;
                break;
            case 0b11:
                /* 64KB page size */
                granule_size = GRANULE_64KB;
                break;
            default:
                assert(!"Invalid TG1 value in TCR_EL1");
                break;
        }
    }

    /*
     * In hardware, everything is a power of two. A 4096-byte page isn't
     * a magic number; it's 2^12. This means you need exactly 12 bits to
     * address every single byte within that page.
     *
     * The naive way to get 12 fron 4096 is to calculate log2(4096) but
     * that's computationally expensive. A much faster way, and how the
     * hardware thinks, is to find the position of the one set bit.
     *
     * 4096 in binary is: 0001 0000 0000 0000 (Bit 12 is set, followed
     * by 12 zeroes).
     *
     * The number of trailing zeroes in a binary number is its
     * logarithm base 2. The COUNT_TRAILING_ZEROES() function
     * is a compiler intrinsic that typically boils down to
     * a single CPU instruction (like TZCNT on x86).
     */
    const uint8_t offset_bits = COUNT_TRAILING_ZEROS(granule_size);

    /*
     * We now need to figure out how many bits are for the index at this
     * level in the page table.
     *
     * A page table is just a big array of 8-byte entires (descriptors).
     * The table itself has to fit perfectly into a page of memory (a granule).
     * So a 4KB page holds a 4KB table.
     *
     * The number of entries in that table is: Granule Size / Entry Size.
     * For a 4KB granule: 4096 bytes / 8 bytes = 512 entries.
     *
     * To index an array of 512 entries we need 9 bits (since 2^9 = 512).
     *
     * log2(Num Entries) = log2(Granule Size / Entry Size)
     * log2(Num Entries) = log2(Granule Size) - log2(Entry Size)
     *
     * We already have log2(Granule Size); that's out `offset_bits`.
     * The `PAGE_TABLE_ENTRY_SHIFT` is a constant for log2(Entry Size).
     * An entry is 8 bytes, and 8 is 2^3, so its log2 is 3.
     *
     * For a 4KB granule:
     * 12 offset bits - 3 bits = 9 index bits.
     *
     */
    const uint8_t page_table_index_bits = offset_bits - PAGE_TABLE_ENTRY_SHIFT;

    /*
     * Next we determine the page table starting level and walk depth based on the
     * virtual address size. The intent is to find the highest table level required
     * to map the address space. A larger VA size requires a deeper walk.
     */
    const uint8_t l3_shift = offset_bits;
    const uint8_t l2_shift = l3_shift + page_table_index_bits;
    const uint8_t l1_shift = l2_shift + page_table_index_bits;
    const uint8_t l0_shift = l1_shift + page_table_index_bits;
    uint8_t page_table_levels = 0;
    uint8_t starting_level = 0;
    switch (granule_size)
    {
        case GRANULE_4KB:
            /* A 4KB granule supports up to a 4-level walk starting at L0. */
            page_table_levels = 3; /* 0..3 inclusive */
            if (virtual_address_size > l0_shift)
            {
                starting_level = 0;
            }
            else if (virtual_address_size > l1_shift)
            {
                starting_level = 1;
            }
            else
            {
                starting_level = 2;
            }
            break;
        case GRANULE_16KB:
        case GRANULE_64KB:
            /* A 16KB and 64KB granule supports up to a 3-level walk starting at L1. */
            page_table_levels = 3; /* 1..3 inclusive */
            if (virtual_address_size > l1_shift)
            {
                starting_level = 1;
            }
            else
            {
                starting_level = 2;
            }
            break;
        default:
            /* This granule size is not supported by the architecture. */
            return -1;
    }

    uint64_t table_address = 0x0;
    if (true == is_ttbr0)
    {
        table_address = vcpu->ttbr0_el1;
    }
    else
    {
        table_address = vcpu->ttbr1_el1;
    }

    /*
     * Begin the multi-level page table walk.
     *
     * The walk starts from the base address of the initial table (L0 or L1,
     * depending on the VA size) and descends level by level. At each level,
     * we extract an index from the GVA, use it to find a descriptor in the
     * current table, and then interpret that descriptor. The descriptor
     * will either point to the next table in the hierarchy, describe
     * the final physical page (a page descriptor), or indicate a fault.
     */
    uint64_t level_index = 0;
    const uint64_t page_table_index_mask = (1ULL << page_table_index_bits) - 1;
    const uint8_t page_table_entry_size = 8;
    for (uint8_t level = starting_level; level <= page_table_levels; ++level)
    {
        switch (level)
        {
            case 0:
                level_index = (gva >> l0_shift) & page_table_index_mask;
                break;
            case 1:
                level_index = (gva >> l1_shift) & page_table_index_mask;
                break;
            case 2:
                level_index = (gva >> l2_shift) & page_table_index_mask;
                break;
            case 3:
                level_index = (gva >> l3_shift) & page_table_index_mask;
                break;
            default:
                assert(!"Invalid page table configuration!");
        }

        const uint64_t level_entry_address = table_address + (level_index * page_table_entry_size);
        uint64_t descriptor = 0;
        guest_mem_readq(memory, level_entry_address, &descriptor);
        uint64_t offset_mask = (1ULL << offset_bits) - 1;
        uint64_t page_offset = gva & offset_mask;
        uint64_t page_address_mask = ~offset_mask;

        /*
         * Is the descriptor valid? Bit [0] of every descriptor is the "valid"
         * bit. If it's 0, the entry is invalid, and the mapping does not exist.
         */
        if (0b0 == (descriptor & 0b1))
        {
            // TODO(GloriousTacoo:arm64): generate page fault.
            return -1;
        }
        /* 
         * At the final level, the only valid descriptor is a Page Descriptor,
         * identified by bits [1:0] being 0b11.
         */
        else if ((level == page_table_levels) && (0b11 == (descriptor & 0b11)))
        {
            /* 
             * The descriptor's upper bits [virtual_address_size:offset_bits]
             * contain the physical base address of the page. We mask out
             * the lower attribute bits to isolate this address.
             */
            uint64_t page_base_address = descriptor & page_address_mask;
            *out_gpa = page_base_address | page_offset;
            return 0;
        }
        /*
         * If this is not the final level, we expect a Table Descriptor, also
         * identified by bits [1:0] being 0b11. This descriptor points to the
         * table for the next level of the walk.
         */
        else if (0b11 == (descriptor & 0b11))
        {
            const uint64_t next_table_mask = ~((1ULL << offset_bits) - 1);
            table_address = descriptor & next_table_mask;
        }
        /*
         * If bits [1:0] are '01', it's a Block Descriptor. These descriptors
         * terminate the walk early, mapping a large, contiguous block of
         * memory (e.g., 2MB at L2). This implementation does not yet
         * support them.
         */
        else if (0b01 == (descriptor & 0b11))
        {
            assert(!"Block descriptors are not supported");
        }
    }
    return -1;
}
}  // namespace pound::kvm::memory
