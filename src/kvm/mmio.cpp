#include "mmio.h"
#include <cassert>
#include <algorithm>

namespace pound::kvm::memory
{
/*
 * This function implements a strict weak ordering comparison on two
 * MMIO ranges, based solely on their starting guest physical address.
 *
 * It is designed to be used with std::lower_bound.
 */
bool mmio_compare_ranges(const mmio_range_t& a, const mmio_range_t& b)
{
    return a.gpa_base < b.gpa_base;
}

int8_t mmio_db_register(mmio_db_t* db, const mmio_range_t range, const mmio_handler_t handler)
{
    assert(nullptr != db);
    assert((db->address_ranges.size() + 1) <= MMIO_REGIONS);

    auto it = std::lower_bound(db->address_ranges.begin(), db->address_ranges.end(), range, mmio_compare_ranges);
    size_t i = it - db->address_ranges.begin();

    /*
     * Scenario: UART is a current region, TIMER is a new region being
     * registered.
     *
     *   [-- UART --]
     *   0x9000     0x9004
     *           [---- TIMER ----]   <-- CONFLICT!
     *           0x9002       0x900A
     */
    if (i > 0)
    {
        if (range.gpa_base < db->address_ranges[i - 1].gpa_end)
        {
            return EADDRESS_OVERLAP;
        }
    }

    /*
     * Scenario: UART is a current region, TIMER is a new region being 
     * registered.
     *
     *   [---- TIMER ----]   <-- CONFLICT!
     *   0x9000       0x9004
     *            [-- UART --]
     *            0x9002     0x900A
     */
    if (i < db->address_ranges.size())
    {
        if (db->address_ranges[i].gpa_base < range.gpa_end)
        {
            return EADDRESS_OVERLAP;
        }
    }

    db->address_ranges.insert(it, range);
    db->handlers.insert(db->handlers.begin() + i, handler);
    return MMIO_SUCCESS;
}

bool mmio_compare_addresses(const mmio_range_t& a, const mmio_range_t& b)
{
    return a.gpa_base < b.gpa_base;
}

int8_t mmio_db_dispatch_write(mmio_db_t* db, kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len)
{
    assert(nullptr != db);
    assert(nullptr != kvm);
    assert(nullptr != data);
    assert(len > 0);

    mmio_range_t search_key = {.gpa_base = gpa, .gpa_end = 0};
    /* Find the first region that starts after the target gpa */
    auto it =
        std::upper_bound(db->address_ranges.begin(), db->address_ranges.end(), search_key, mmio_compare_addresses);

    /* If `it` is the beginning, then the gpa is smaller than all known regions. */
    if (db->address_ranges.begin() == it)
    {
        return ENOT_HANDLED;
    }

    mmio_range_t candidate = *(it - 1);
    /* base <= gpa < end */
    if ((candidate.gpa_base <= gpa) && (gpa < candidate.gpa_end))
    {
        size_t i = (it - 1) - db->address_ranges.begin();
        if (nullptr == db->handlers[i].write)
        {
            return EACCESS_DENIED;
        }

        db->handlers[i].write(kvm, gpa, data, len);
        return MMIO_SUCCESS;
    }

    /* The gpa is not in any mmio region. */
    return ENOT_HANDLED;
}

int8_t mmio_db_dispatch_read(mmio_db_t* db, kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len)
{
    assert(nullptr != db);
    assert(nullptr != kvm);
    assert(nullptr != data);
    assert(len > 0);

    mmio_range_t search_key = {.gpa_base = gpa, .gpa_end = 0};
    /* Find the first region that starts after the target gpa */
    auto it =
        std::upper_bound(db->address_ranges.begin(), db->address_ranges.end(), search_key, mmio_compare_addresses);

    /* If `it` is the beginning, then the gpa is smaller than all known regions. */
    if (db->address_ranges.begin() == it)
    {
        return ENOT_HANDLED;
    }

    mmio_range_t candidate = *(it - 1);
    /* base <= gpa < end */
    if ((candidate.gpa_base <= gpa) && (gpa < candidate.gpa_end))
    {
        size_t i = (it - 1) - db->address_ranges.begin();
        if (nullptr == db->handlers[i].read)
        {
            return EACCESS_DENIED;
        }

        db->handlers[i].read(kvm, gpa, data, len);
        return MMIO_SUCCESS;
    }

    /* The gpa is not in any mmio region. */
    return ENOT_HANDLED;
}
}  // namespace pound::kvm::memory
