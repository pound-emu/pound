#ifndef POUND_GPU_SM86_WARP_H
#define POUND_GPU_SM86_WARP_H

#include "attributes.h"
#include "decoder.h"
#include "mmu.h"
#include <stdbool.h>
#include <stdint.h>

#define SM86_WARP_SIZE                32
#define SM86_MAX_GPRS                 256
#define SM86_MAX_UGPRS                64
#define SM86_MAX_PREDICATES           8
#define SM86_RECONVERGENCE_STACK_SIZE 32

typedef struct sm86_cta_t sm86_cta_t;

typedef enum
{
    SM86_TOKEN_DIVERGENCE = 0,
    SM86_TOKEN_SYNC       = 1
} sm86_token_type_t;

typedef struct
{
    uint32_t          pc;
    uint32_t          active_mask;
    sm86_token_type_t type;
} sm86_reconvergence_token_t;

typedef struct POUND_ALIGNED(64)
{
    uint32_t pc;
    uint32_t execution_mask; // 1 bit per active thread in the warp.

    // Each element is 1 predicate register for all 32 threads. Bit N -> Thread N. Index 7 is the
    // True predicate.
    uint32_t predicates[SM86_MAX_PREDICATES];

    // gprs[register_index][thread_index]
    POUND_ALIGNED(64) uint32_t gprs[SM86_MAX_GPRS][SM86_WARP_SIZE];

    uint32_t                   uniform_gprs[SM86_MAX_UGPRS];
    uint8_t                    uniform_predicates; // 1 bit per UPred
    uint8_t                    pad[3];
    uint32_t                   reconvergence_depth;
    sm86_reconvergence_token_t reconvergence_stack[SM86_RECONVERGENCE_STACK_SIZE];
} sm86_warp_t;

POUND_HOT bool sm86_warp_execute(sm86_cta_t                       *cta,
                                 uint32_t                          warp_id,
                                 const sm86_mmu_t                 *mmu,
                                 const sm86_decoded_instruction_t *instructions,
                                 uint32_t                          max_cycles);

#endif // POUND_GPU_SM86_WARP_H

/*** end of file ***/
