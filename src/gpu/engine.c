#include "engine.h"
#include <stdlib.h>
#include <string.h>

gpu_engine_t *
gpu_engine_create(void)
{
    gpu_engine_t *engine = calloc(1, sizeof(gpu_engine_t));
    return engine;
}

void
gpu_engine_destroy(gpu_engine_t *engine)
{
    if (engine != NULL)
    {
        free(engine);
    }
}

bool
gpu_engine_map_memory(gpu_engine_t *POUND_RESTRICT engine,
                      const uint64_t               virtual_address,
                      const uint64_t               size,
                      void *POUND_RESTRICT         host_ptr)
{
    if ((virtual_address & SM86_PAGE_MASK) != 0 || (size & SM86_PAGE_MASK) != 0)
    {
        return false;
    }

    const uint32_t           start_page  = virtual_address >> SM86_PAGE_SHIFT & SM86_PAGE_MAX - 1;
    const uint32_t           num_pages   = (uint32_t)(size >> SM86_PAGE_SHIFT);
    uint8_t *POUND_RESTRICT  host_bytes  = host_ptr;
    uint8_t **POUND_RESTRICT page_cursor = &engine->mmu.host_pages[start_page];

    for (uint32_t i = 0; i < num_pages; ++i)
    {
        *page_cursor++ = host_bytes;
        host_bytes += 1U << SM86_PAGE_SHIFT; // Advance by 64KB.
    }

    return true;
}

void
gpu_engine_unmap_memory(gpu_engine_t *engine, const uint64_t virtual_address, const uint64_t size)
{
    if ((virtual_address & SM86_PAGE_MASK) != 0 || (size & SM86_PAGE_MASK) != 0)
    {
        return;
    }

    const uint32_t start_page  = virtual_address >> SM86_PAGE_SHIFT & SM86_PAGE_MAX - 1;
    const uint32_t num_pages   = (uint32_t)(size >> SM86_PAGE_SHIFT);
    uint8_t      **page_cursor = &engine->mmu.host_pages[start_page];

    for (uint32_t i = 0; i < num_pages; ++i)
    {
        *page_cursor++ = NULL;
    }
}

void
gpu_engine_launch_cta(gpu_engine_t *POUND_RESTRICT                     engine,
                      uint32_t                                         num_warps,
                      const sm86_decoded_instruction_t *POUND_RESTRICT instructions)
{
    uint32_t active_mask = 0;

    for (uint32_t i = 0; i < num_warps; ++i)
    {
        engine->cta.warp_states[i]          = SM86_WARP_STATE_READY;
        engine->cta.warps[i].execution_mask = 0xFFFFFFFFULL;
        engine->cta.warps[i].predicates[7]  = 0xFFFFFFFFULL;
        active_mask |= 1U << i;
    }

    engine->cta.active_warps_mask = active_mask;
    sm86_cta_execute(&engine->cta, &engine->mmu, instructions);
}