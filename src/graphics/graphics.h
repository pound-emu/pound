#ifndef POUND_GRAPHICS_H
#define POUND_GRAPHICS_H

#include "SDL3/SDL_video.h"
#include "attributes.h"
#include "errors.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS 1
#include "cimgui.h"
#include "cimgui_impl.h"

/// THIS GRAPHICS SUBSYSTEM IS BEING DESIGNED AROUND 4GB VRAM.

// Keep at 2 for the VRAM target.
#define GRAPHICS_FRAMES_IN_FLIGHT 2

/// Upper bound on acquired swapchain images.
#define GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY 8

/// VK_MAX_PHYSICAL_DEVICE_NAME_SIZE
#define GRAPHICS_DEVICE_NAME_CAPACITY 256

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

typedef enum
{
    GRAPHICS_FEATURE_NONE          = 0,
    GRAPHICS_FEATURE_MEMORY_BUDGET = 1U << 0, // <-- VK_EXT_memory_budget
} graphics_feature_flags_t;

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

POUND_ALIGNED(64) typedef struct
{
    VkCommandBuffer command_buffer;
    VkFence         in_flight_fence;
    VkSemaphore     image_available_semaphore;
    uint64_t        submission_index;
    uint32_t        image_index;
    uint32_t        pad0;
    char            pad[24];
} graphics_frame_t;

static_assert(64 == sizeof(graphics_frame_t), "Struct size mismatch");

POUND_ALIGNED(64) typedef struct
{
    VkImage       image;
    VkImageView   image_view;
    VkFramebuffer framebuffer;
    VkSemaphore   render_finished_semaphore;
    char          pad[32];
} graphics_swapchain_image_t;

static_assert(64 == sizeof(graphics_swapchain_image_t), "Struct size mismatch");

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

    graphics_capabilities_t    capabilities;
    graphics_frame_t           frames[GRAPHICS_FRAMES_IN_FLIGHT];
    graphics_swapchain_image_t swapchain_images[GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY];

    VkInstance               instance;
    VkPhysicalDevice         physical_device;
    VkDevice                 device;
    VkSwapchainKHR           swapchain;
    VkRenderPass             render_pass;
    VkExtent2D               swapchain_extent;
    VkSurfaceKHR             surface;
    VkSurfaceFormatKHR       surface_format;
    VkQueue                  graphics_queue;
    VkQueue                  present_queue;
    VkCommandPool            command_pool;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPresentModeKHR         present_mode;
    SDL_Window              *window;
    ImGuiContext            *imgui_context;
    char                     device_name[GRAPHICS_DEVICE_NAME_CAPACITY];
} graphics_state_t;

static_assert(64 == sizeof(graphics_capabilities_t), "Struct size mismatch");

extern graphics_state_t g_graphics_state;

POUND_COLD pound_error_t graphics_subsystem_init(const graphics_config_t *POUND_RESTRICT config);
POUND_COLD void          graphics_subsystem_destroy(void);

POUND_HOT pound_error_t graphics_subsystem_begin_frame(void);
POUND_HOT pound_error_t graphics_subsystem_end_frame(void);
POUND_HOT pound_error_t graphics_subsystem_recreate_swapchain(void);

/// Returns true if ImGui consumed the event.
POUND_HOT bool           graphics_subsystem_process_event(SDL_Event *restrict event);
POUND_COLD pound_error_t graphics_subsystem_drain_pipeline(void);

POUND_HOT SDL_Window   *graphics_subsystem_get_window(void);
POUND_HOT ImGuiContext *graphics_subsystem_get_imgui_context(void);

#endif // POUND_GRAPHICS_H