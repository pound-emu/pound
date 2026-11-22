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

typedef enum
{
    GRAPHICS_STATE_FLAG_NONE            = 0,
    GRAPHICS_STATE_FLAG_FRAME_ACTIVE    = 1U << 0,
    GRAPHICS_STATE_FLAG_SWAPCHAIN_DIRTY = 1U << 1,
    GRAPHICS_STATE_FLAG_MINIMIZED       = 1U << 2,
    GRAPHICS_STATE_FLAG_VALIDATION      = 1U << 3,
    GRAPHICS_STATE_FLAG_VSYNC           = 1U << 4,
} graphics_state_flags_t;

POUND_ALIGNED(64) typedef struct
{
    // -------------------------------------
    // ONE CACHE LINE. THIS INFORMATION IS ACCESSED PER FRAME.
    uint64_t frame_index;
    uint64_t submit_count;
    uint64_t present_count;
    uint32_t current_frame_slot;
    uint32_t drawable_width;
    uint32_t drawable_height;
    uint32_t swapchain_image_count;
    /// Bitmask of [`graphics_state_flags_t`].
    uint32_t      state_flags;
    uint32_t      pad0;
    pound_error_t status;
    bool          initialized;
    char          pad1[11];
    // -------------------------------------
} graphics_state_t;

static_assert(64 == sizeof(graphics_capabilities_t), "Struct size mismatch");

extern graphics_state_t g_graphics_state;

POUND_COLD pound_error_t graphics_subsystem_init(const graphics_config_t *POUND_RESTRICT config);

#endif // POUND_GRAPHICS_H
