#include "cta.h"

void
sm86_cta_execute(sm86_cta_t *POUND_RESTRICT                       cta,
                 const sm86_mmu_t *POUND_RESTRICT                 mmu,
                 const sm86_decoded_instruction_t *POUND_RESTRICT instructions)
{
    uint32_t active_warp_mask = cta->active_warps_mask;

    // This is a Cooperative Round Robin Scheduler.
    while (POUND_LIKELY(active_warp_mask != 0))
    {
        uint32_t current_mask = active_warp_mask;

        // Find the lowest set bit, execute that warp, then clear the bit.
        while (current_mask != 0)
        {
            const uint32_t warp_id = (uint32_t)__builtin_ctz(current_mask);

            if (POUND_LIKELY(SM86_WARP_STATE_READY == cta->warp_states[warp_id]))
            {
                const uint32_t max_cycles = 1024;
                const bool exited = sm86_warp_execute(cta, warp_id, mmu, instructions, max_cycles);

                if (POUND_UNLIKELY(true == exited))
                {
                    cta->warp_states[warp_id] = SM86_WARP_STATE_EXITED;
                    active_warp_mask &= ~(1U << warp_id);
                }
            }

            current_mask &= ~(1U << warp_id);
        }

        // TODO: Add deadlock detection check.
    }

    cta->active_warps_mask = active_warp_mask;
}