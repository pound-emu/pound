#ifndef POUND_GRAPHICS_H
#define POUND_GRAPHICS_H

#include "attributes.h"
#include "errors.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    const char *window_title;
    uint32_t    window_width;
    uint32_t    window_height;
    bool        enable_validation_layers;
    bool        enable_fifo_vsync;
    char        pad[6];
} graphics_config_t;

static_assert(24 == sizeof(graphics_config_t), "Struct size mismatch");

POUND_ALIGNED(64) typedef struct
{
    uint64_t device_local_heap_bytes;
    uint64_t minimum_uniform_buffer_alignment;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t api_version;
    uint32_t max_image_dimension_2d;
    uint32_t max_memory_allocation_count;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
    uint32_t swapchain_image_count;
    uint32_t feature_flags;
    char     pad[12];
} graphics_capabilities_t;

static_assert(64 == sizeof(graphics_capabilities_t), "Struct size mismatch");

POUND_COLD pound_error_t graphics_subsystem_init(const graphics_config_t *POUND_RESTRICT config);

#endif // POUND_GRAPHICS_H
