#ifndef POUND_GPU_SM86_MMU_H
#define POUND_GPU_SM86_MMU_H

#include "attributes.h"
#include <stdint.h>

// 64KB Pages.
// A 40-bit virtual address space requires a 16MB page table. This can fit in CPU L3 cache.
#define SM86_PAGE_SHIFT 16
#define SM86_PAGE_MASK  0XFFFF
#define SM86_PAGE_MAX   (1ULL << 24)

typedef struct
{
    /// Pointer is NULL if page is unmapped.
    uint8_t *host_pages[SM86_PAGE_MAX];
} sm86_mmu_t;

#endif // POUND_GPU_SM86_MMU_H
