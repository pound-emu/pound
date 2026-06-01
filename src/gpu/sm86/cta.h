#ifndef POUND_GPU_SM86_CTA_H
#define POUND_GPU_SM86_CTA_H

#include "attributes.h"
#include "mmu.h"
#include "warp.h"
#include <stdbool.h>
#include <stdint.h>

#define SM86_MAX_WARPS_PER_CTA  32
#define SM86_MAX_BARRIERS       16
#define SM86_SHARED_MEMORY_SIZE (96 * 1024) // 96 KB

typedef enum
{
    SM86_WARP_STATE_INACTIVE = 0,
    SM86_WARP_STATE_READY,
    SM86_WARP_STATE_BLOCKED, // Waiting on a barrier
    SM86_WARP_STATE_EXITED,
} sm86_warp_state_t;

typedef struct POUND_ALIGNED(64) sm86_cta_t
{
    sm86_warp_t       warps[SM86_MAX_WARPS_PER_CTA];
    sm86_warp_state_t warp_states[SM86_MAX_WARPS_PER_CTA];
    uint32_t          active_warps_mask; // 1 bit per active warp.

    // barrier_arrival_mask[i] has bit N set if Warp N has reached Barrier i.
    uint32_t barrier_arrival_mask[SM86_MAX_BARRIERS];

    // barrier_expected_mask[i] has bit N set if Warp N is expected to reach Barrier i.
    uint32_t barrier_expected_mask[SM86_MAX_BARRIERS];

    POUND_ALIGNED(64) uint8_t shared_memory[SM86_SHARED_MEMORY_SIZE];
} sm86_cta_t;

void sm86_cta_execute(sm86_cta_t                       *cta,
                      const sm86_mmu_t                 *mmu,
                      const sm86_decoded_instruction_t *instructions);

#endif // POUND_GPU_SM86_CTA_H
