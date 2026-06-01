#ifndef POUND_GPU_ENGINE_H
#define POUND_GPU_ENGINE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "sm86/cta.h"
#include "sm86/mmu.h"

    typedef struct
    {
        sm86_mmu_t mmu;
        sm86_cta_t cta;
        char       pad[4];
    } gpu_engine_t;

    gpu_engine_t *gpu_engine_create(void);
    void          gpu_engine_destroy(gpu_engine_t *engine);
    bool          gpu_engine_map_memory(gpu_engine_t *engine,
                                        uint64_t      virtual_address,
                                        uint64_t      size,
                                        void         *host_ptr);
    void gpu_engine_unmap_memory(gpu_engine_t *engine, uint64_t virtual_address, uint64_t size);

    /// Launch a single CTA for testing purposes.
    void gpu_engine_launch_cta(gpu_engine_t                     *engine,
                               uint32_t                          num_warps,
                               const sm86_decoded_instruction_t *instructions);

#ifdef __cplusplus
}
#endif

#endif // POUND_GPU_ENGINE_H

/*** end of file ***/