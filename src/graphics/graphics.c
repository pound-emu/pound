#include "graphics.h"

#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_vulkan.h"
#include "log.h"
#include "memory/memory.h"
#include "swapchain.h"

#define GRAPHICS_MAX_INSTANCE_EXTENSIONS 64
#define GRAPHICS_MAX_PHYSICAL_DEVICES    8
#define GRAPHICS_MAX_QUEUE_FAMILIES      32
#define GRAPHICS_VALIDATION_LAYER_NAME   "VK_LAYER_KHRONOS_validation"

graphics_state_t g_graphics_state = { 0 };

POUND_COLD static pound_error_t sdl_window_create(graphics_state_t *POUND_RESTRICT        state,
                                                  const graphics_config_t *POUND_RESTRICT config);
POUND_COLD static pound_error_t imgui_init(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_instance_create(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_debug_messenger_create(
    graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_surface_create(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_physical_device_select(
    graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_device_create(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_frame_objects_create(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static void vulkan_frame_objects_destroy(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static bool vulkan_is_graphics_layer_available(const char *POUND_RESTRICT layer_name);
POUND_COLD static bool vulkan_is_device_extension_supported(VkPhysicalDevice           device,
                                                            const char *POUND_RESTRICT name);

static VKAPI_ATTR VkBool32 VKAPI_CALL
graphics_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                        VkDebugUtilsMessageTypeFlagsEXT             types,
                        const VkDebugUtilsMessengerCallbackDataEXT *data,
                        void                                       *user_data);

POUND_COLD pound_error_t
graphics_subsystem_init(const graphics_config_t *POUND_RESTRICT config)
{
    graphics_state_t *POUND_RESTRICT state = &g_graphics_state;

    if (POUND_UNLIKELY(NULL == config))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: config is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(state->initialized))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: graphics system is already "
                        "initialized.");
        return POUND_ERROR_GRAPHICS_ALREADY_INITIALIZED;
    }

    if (POUND_UNLIKELY(0 == config->window_width || 0 == config->window_height))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: window size %ux%u must be non-zero.",
                        config->window_width,
                        config->window_height);
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(NULL == config->window_title))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: config->window_title is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    (void)memset(state, 0, sizeof(graphics_state_t));
    state->status = POUND_SUCCESS;

    if (config->enable_validation_layers)
    {
        state->state_flags |= GRAPHICS_STATE_FLAG_VALIDATION;
    }

    if (config->enable_fifo_vsync)
    {
        state->state_flags |= GRAPHICS_STATE_FLAG_VSYNC;
    }

    POUND_LOG_DEBUG(&thread_logger, "Configuring graphics subsystem...");

    pound_error_t error = sdl_window_create(state, config);

    if (POUND_UNLIKELY(POUND_SUCCESS != error))
    {
        graphics_subsystem_destroy();
        return error;
    }

    error = vulkan_instance_create(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    if ((state->state_flags & GRAPHICS_STATE_FLAG_VALIDATION) != 0)
    {
        error = vulkan_debug_messenger_create(state);

        if (POUND_UNLIKELY(error != POUND_SUCCESS))
        {
            graphics_subsystem_destroy();
            return error;
        }
    }

    error = vulkan_surface_create(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    error = vulkan_physical_device_select(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    error = vulkan_device_create(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    error = vulkan_frame_objects_create(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    error = swapchain_create(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    error = imgui_init(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        graphics_subsystem_destroy();
        return error;
    }

    state->initialized = true;
    POUND_LOG_INFO(&thread_logger,
                   "Successfully configured graphics subsystem on '%s' (%ux%u, %u images).",
                   state->device_name,
                   state->swapchain_extent.width,
                   state->swapchain_extent.height,
                   state->swapchain_image_count);
    state->status = POUND_SUCCESS;
    return state->status;
}

POUND_COLD void
graphics_subsystem_destroy(void)
{
    graphics_state_t *POUND_RESTRICT state = &g_graphics_state;

    if (state->device != NULL)
    {
        const VkResult result = vkDeviceWaitIdle(state->device);

        if (POUND_UNLIKELY(result != VK_SUCCESS))
        {
            POUND_LOG_WARN(&thread_logger,
                           "vkDeviceWaitIdle returned %d during shutdown, continuing.",
                           (int)result);
        }
    }

    if (state->imgui_context != NULL)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        igDestroyContext(state->imgui_context);
        state->imgui_context = NULL;
    }

    swapchain_destroy(state);

    if (state->device != NULL && state->swapchain != NULL)
    {
        vkDestroySwapchainKHR(state->device, state->swapchain, NULL);
        state->swapchain = NULL;
    }

    vulkan_frame_objects_destroy(state);

    if (state->device != NULL && state->command_pool != NULL)
    {
        vkDestroyCommandPool(state->device, state->command_pool, NULL);
        state->command_pool = NULL;
    }

    if (state->device != NULL)
    {
        vkDestroyDevice(state->device, NULL);
        state->device = NULL;
    }

    if (state->instance != NULL && state->surface != NULL)
    {
        vkDestroySurfaceKHR(state->instance, state->surface, NULL);
        state->surface = NULL;
    }

    if (state->instance != NULL && state->debug_messenger != NULL)
    {
        const PFN_vkDestroyDebugUtilsMessengerEXT destroy
            = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                state->instance, "vkDestroyDebugUtilsMessengerEXT");

        if (destroy != NULL)
        {
            destroy(state->instance, state->debug_messenger, NULL);
        }
        else
        {
            POUND_LOG_WARN(&thread_logger,
                           "vkDestroyDebugUtilsMessengerEXT() could not be resolved, leaking "
                           "the messenger.");
        }

        state->debug_messenger = NULL;
    }

    if (state->instance != NULL)
    {
        vkDestroyInstance(state->instance, NULL);
        state->instance = NULL;
    }

    if (state->window != NULL)
    {
        SDL_DestroyWindow(state->window);
        state->window = NULL;
    }

    (void)memset(state, 0, sizeof(graphics_state_t));
    POUND_LOG_DEBUG(&thread_logger, "Graphics subsystem destroyed.");
}

POUND_COLD static pound_error_t
sdl_window_create(graphics_state_t *POUND_RESTRICT        state,
                  const graphics_config_t *POUND_RESTRICT config)
{
    if (0 == SDL_WasInit(SDL_INIT_VIDEO))
    {
        if (false == SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: SDL_InitSubSystem failed because %s.",
                            SDL_GetError());
            state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }
    }

    if (false == SDL_SetHint(SDL_HINT_APP_NAME, config->window_title))
    {
        POUND_LOG_WARN(
            &thread_logger, "SDL_SetHint(SDL_HINT_APP_NAME) failed because %s", SDL_GetError());
    }

    state->window = SDL_CreateWindow(config->window_title,
                                     (int)config->window_height,
                                     (int)config->window_width,
                                     SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
                                         | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);

    if (NULL == state->window)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SDL_CreateWindow() failed because %s.",
                        SDL_GetError());
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    int width  = 0;
    int height = 0;

    if (false == SDL_GetWindowSizeInPixels(state->window, &width, &height))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SDL_GetWindowSizeInPixels() failed because %s.",
                        SDL_GetError());
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (width <= 0 || height <= 0)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: drawable size %dx%d is not positive.",
                        width,
                        height);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    state->drawable_height = (uint32_t)height;
    state->drawable_width  = (uint32_t)width;
    return state->status;
}

POUND_COLD static pound_error_t
imgui_init(graphics_state_t *POUND_RESTRICT state)
{
    state->imgui_context = igCreateContext(NULL);

    if (NULL == state->imgui_context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: igCreateContext() returned NULL.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (false == ImGui_ImplSDL3_InitForVulkan(state->window))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: ImGui_ImplSDL3_InitForVulkan failed.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    ImGui_ImplVulkan_InitInfo info    = { 0 };
    info.Instance                     = state->instance;
    info.PhysicalDevice               = state->physical_device;
    info.Device                       = state->device;
    info.QueueFamily                  = state->capabilities.graphics_queue_family;
    info.Queue                        = state->graphics_queue;
    info.PipelineInfoMain.RenderPass  = state->render_pass;
    info.PipelineInfoMain.Subpass     = 0;
    info.MinImageCount                = state->swapchain_image_count;
    info.ImageCount                   = state->swapchain_image_count;
    info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.DescriptorPoolSize           = 1000;

    if (false == ImGui_ImplVulkan_Init(&info))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: ImGui_ImplVulkan_Init failed.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    ImGuiIO *io = igGetIO_ContextPtr(state->imgui_context);

    if (NULL == io)
    {
        POUND_LOG_WARN(&thread_logger,
                       "ImGui IO is unavailable after initialization, continuing with defaults.");
    }
    else
    {
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io->ConfigErrorRecovery             = true;
        io->ConfigErrorRecoveryEnableAssert = false;
    }

    igStyleColorsDark(NULL);
    return state->status;
}

POUND_COLD static pound_error_t
vulkan_instance_create(graphics_state_t *POUND_RESTRICT state)
{
    if (state->status != POUND_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: status->status != POUND_SUCCESS.");
    }

    uint32_t           sdl_count      = 0;
    const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_count);

    if (NULL == sdl_extensions)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SDL_Vulkan_GetInstanceExtensions failed because %s.",
                        SDL_GetError());
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (sdl_count + 1U > GRAPHICS_MAX_INSTANCE_EXTENSIONS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SDL reported %u instance extensions, capacity is %d.",
                        sdl_count,
                        GRAPHICS_MAX_INSTANCE_EXTENSIONS);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    const char                 *extensions[GRAPHICS_MAX_INSTANCE_EXTENSIONS] = { 0 };
    const char **POUND_RESTRICT extensions_cursor                            = extensions;

    for (uint32_t i = 0; i < sdl_count; ++i)
    {
        *extensions_cursor++ = sdl_extensions[i];
    }

    uint32_t   extension_count = sdl_count;
    const bool validation      = (state->state_flags & GRAPHICS_STATE_FLAG_VALIDATION) != 0;

    if (validation == true)
    {
        if (false == vulkan_is_graphics_layer_available(GRAPHICS_VALIDATION_LAYER_NAME))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: validation was requested but %s is not installed.",
                            GRAPHICS_VALIDATION_LAYER_NAME);
            state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }
        *extensions_cursor++ = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        ++extension_count;
    }

    VkApplicationInfo application  = { 0 };
    application.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName   = "Pound";
    application.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    application.pEngineName        = "Pound";
    application.engineVersion      = VK_MAKE_API_VERSION(0, 0, 1, 0);
    application.apiVersion         = VK_API_VERSION_1_1;

    VkInstanceCreateInfo create_info    = { 0 };
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &application;
    create_info.enabledExtensionCount   = extension_count;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.enabledLayerCount       = validation ? 1U : 0U;
    create_info.ppEnabledLayerNames
        = validation ? &(const char *) { GRAPHICS_VALIDATION_LAYER_NAME } : NULL;

    const VkResult result = vkCreateInstance(&create_info, NULL, &state->instance);

    if (POUND_UNLIKELY(VK_SUCCESS != result))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkCreateInstance returned %d.", (int)result);
        state->instance = NULL;
        state->status   = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    return state->status;
}

POUND_COLD static pound_error_t
vulkan_debug_messenger_create(graphics_state_t *POUND_RESTRICT state)
{
    if (state->status != POUND_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->status != POUND_SUCCESS.");
        return state->status;
    }

    const PFN_vkCreateDebugUtilsMessengerEXT create
        = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            state->instance, "vkCreateDebugUtilsMessengerEXT");

    if (NULL == create)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkCreateDebugUtilsMessengerEXT could not be resolved.");
        return state->status;
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info = { 0 };
    create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = graphics_debug_callback;

    const VkResult result = create(state->instance, &create_info, NULL, &state->debug_messenger);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkCreateDebugUtilsMessengerEXT() returned %d.",
                        (int)result);
        state->debug_messenger = NULL;
        state->status          = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    return state->status;
}

POUND_COLD static pound_error_t
vulkan_surface_create(graphics_state_t *POUND_RESTRICT state)
{
    if (state->status != POUND_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->status != POUND_SUCCESS.");
        return state->status;
    }

    if (false == SDL_Vulkan_CreateSurface(state->window, state->instance, NULL, &state->surface))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SDL_Vulkan_CreateSurface failed because %s.",
                        SDL_GetError());
        state->surface = NULL;
        state->status  = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    return state->status;
}

POUND_COLD static pound_error_t
vulkan_physical_device_select(graphics_state_t *POUND_RESTRICT state)
{
    uint32_t count  = 0U;
    VkResult result = vkEnumeratePhysicalDevices(state->instance, &count, NULL);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkEnumeratePhysicalDevices() returned %d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (POUND_UNLIKELY(0 == count))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: no Vulkan physical devices were reported.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    VkPhysicalDevice devices[GRAPHICS_MAX_PHYSICAL_DEVICES] = { 0 };

    if (count > GRAPHICS_MAX_PHYSICAL_DEVICES)
    {
        POUND_LOG_WARN(&thread_logger,
                       "Reported %u physical devices, only inspecting the first %d.",
                       count,
                       GRAPHICS_MAX_PHYSICAL_DEVICES);
        count = GRAPHICS_MAX_PHYSICAL_DEVICES;
    }

    result = vkEnumeratePhysicalDevices(state->instance, &count, devices);

    if (POUND_UNLIKELY(VK_SUCCESS != result))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkEnumeratePhysicalDevices returned %d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    const VkPhysicalDevice *POUND_RESTRICT device_cursor                            = devices;
    VkPhysicalDevice                       best_device                              = NULL;
    uint32_t                               best_score                               = 0;
    graphics_capabilities_t                best_capabilities                        = { 0 };
    char                                   best_name[GRAPHICS_DEVICE_NAME_CAPACITY] = { 0 };

    for (uint32_t i = 0; i < count; ++i)
    {
        const VkPhysicalDevice device       = *device_cursor;
        uint32_t               family_count = 0U;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, NULL);

        if (0 == family_count)
        {
            continue;
        }

        VkQueueFamilyProperties families[GRAPHICS_MAX_QUEUE_FAMILIES];

        if (family_count > GRAPHICS_MAX_QUEUE_FAMILIES)
        {
            family_count = GRAPHICS_MAX_QUEUE_FAMILIES;
        }

        vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families);

        uint32_t graphics_family = UINT32_MAX;
        uint32_t present_family  = UINT32_MAX;

        const VkQueueFamilyProperties *POUND_RESTRICT family_cursor = families;

        for (uint32_t f = 0; f < family_count; ++f)
        {
            const VkQueueFamilyProperties properties = *family_cursor;

            if (UINT32_MAX == graphics_family
                && (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                graphics_family = f;
            }

            VkBool32       supported = VK_FALSE;
            const VkResult present
                = vkGetPhysicalDeviceSurfaceSupportKHR(device, f, state->surface, &supported);

            if (present != VK_SUCCESS)
            {
                POUND_LOG_WARN(&thread_logger,
                               "vkGetPhysicalDeviceSurfaceSupportKHR() returned %d for device %u "
                               "family %u, skipping that family.",
                               (int)present,
                               i,
                               f);
                continue;
            }

            if (UINT32_MAX == present_family && VK_TRUE == supported)
            {
                present_family = f;
            }
        }

        if (UINT32_MAX == graphics_family || UINT32_MAX == present_family)
        {
            continue;
        }

        if (false == vulkan_is_device_extension_supported(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            continue;
        }

        uint32_t format_count = 0U;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, state->surface, &format_count, NULL);

        if (result != VK_SUCCESS || 0 == format_count)
        {
            continue;
        }

        VkPhysicalDeviceProperties properties = { 0 };
        vkGetPhysicalDeviceProperties(device, &properties);

        VkPhysicalDeviceMemoryProperties memory_properties = { 0 };
        vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

        uint64_t device_local = 0U;

        for (uint32_t h = 0; h < memory_properties.memoryHeapCount; ++h)
        {
            if ((memory_properties.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
            {
                device_local += memory_properties.memoryHeaps[h].size;
            }
        }

        // Prefer discrete GPUs, then larger device-local heaps.
        uint32_t score
            = (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == properties.deviceType) ? 1000000U : 0U;
        score += (uint32_t)(device_local / (1024ULL * 1024ULL * 256ULL));

        if (score <= best_score && NULL != best_device)
        {
            continue;
        }

        graphics_capabilities_t capabilities = { 0 };
        capabilities.device_local_heap_bytes = device_local;
        capabilities.minimum_uniform_buffer_alignment
            = properties.limits.minUniformBufferOffsetAlignment;
        capabilities.vendor_id                   = properties.vendorID;
        capabilities.device_id                   = properties.deviceID;
        capabilities.api_version                 = properties.apiVersion;
        capabilities.max_image_dimension_2d      = properties.limits.maxImageDimension2D;
        capabilities.max_memory_allocation_count = properties.limits.maxMemoryAllocationCount;
        capabilities.graphics_queue_family       = graphics_family;
        capabilities.present_queue_family        = present_family;
        capabilities.feature_flags               = GRAPHICS_FEATURE_NONE;

        if (vulkan_is_device_extension_supported(device, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)
            == true)
        {
            capabilities.feature_flags |= GRAPHICS_FEATURE_MEMORY_BUDGET;
        }

        best_device       = device;
        best_score        = score;
        best_capabilities = capabilities;
        (void)memcpy(best_name, properties.deviceName, GRAPHICS_DEVICE_NAME_CAPACITY);
    }

    if (NULL == best_device)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: no physical device supports graphics, presentation, "
                        "and VK_KHR_swapchain.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    state->physical_device = best_device;
    state->capabilities    = best_capabilities;
    (void)memcpy(state->device_name, best_name, GRAPHICS_DEVICE_NAME_CAPACITY);

    POUND_LOG_INFO(
        &thread_logger,
        "Selected '%s': graphics family %u, present family %u, %llu MiB device-local.",
        state->device_name,
        state->capabilities.graphics_queue_family,
        state->capabilities.present_queue_family,
        (unsigned long long)(state->capabilities.device_local_heap_bytes / (1024ULL * 1024ULL)));
    return state->status;
}

POUND_COLD pound_error_t
vulkan_device_create(graphics_state_t *POUND_RESTRICT state)
{
    if (state->status != POUND_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->status != POUND_SUCCESS.");
        return state->status;
    }

    const float             priority = 1.0F;
    VkDeviceQueueCreateInfo queue_infos[2];
    (void)memset(queue_infos, 0, sizeof(queue_infos));

    queue_infos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos[0].queueFamilyIndex = state->capabilities.graphics_queue_family;
    queue_infos[0].queueCount       = 1;
    queue_infos[0].pQueuePriorities = &priority;
    uint32_t queue_info_count       = 1;

    if (state->capabilities.present_queue_family != state->capabilities.graphics_queue_family)
    {
        queue_infos[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_infos[1].queueFamilyIndex = state->capabilities.present_queue_family;
        queue_infos[1].queueCount       = 1;
        queue_infos[1].pQueuePriorities = &priority;
        queue_info_count                = 2;
    }

    const char *extensions[2];
    extensions[0]            = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    uint32_t extension_count = 1U;

    if ((state->capabilities.feature_flags & GRAPHICS_FEATURE_MEMORY_BUDGET) != 0)
    {
        extensions[extension_count++] = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
    }

    VkDeviceCreateInfo create_info      = { 0 };
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount    = queue_info_count;
    create_info.pQueueCreateInfos       = queue_infos;
    create_info.enabledExtensionCount   = extension_count;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.pEnabledFeatures        = NULL;

    const VkResult result
        = vkCreateDevice(state->physical_device, &create_info, NULL, &state->device);
    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkCreateDevice() returned %d.", (int)result);
        state->device = NULL;
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    vkGetDeviceQueue(
        state->device, state->capabilities.graphics_queue_family, 0, &state->graphics_queue);
    vkGetDeviceQueue(
        state->device, state->capabilities.present_queue_family, 0, &state->present_queue);

    if (NULL == state->graphics_queue || NULL == state->present_queue)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetDeviceQueue() returned a NULL queue.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    VkCommandPoolCreateInfo pool_info = { 0 };
    pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex        = state->capabilities.graphics_queue_family;

    const VkResult pool_result
        = vkCreateCommandPool(state->device, &pool_info, NULL, &state->command_pool);

    if (POUND_UNLIKELY(pool_result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkCreateCommandPool returned %d.",
                        (int)pool_result);
        state->command_pool = NULL;
        state->status       = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    return state->status;
}

POUND_COLD static pound_error_t
vulkan_frame_objects_create(graphics_state_t *POUND_RESTRICT state)
{
    if (state->status != POUND_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->status != POUND_SUCCESS.");
        return state->status;
    }

    VkCommandBufferAllocateInfo allocate_info = { 0 };
    allocate_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool                 = state->command_pool;
    allocate_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount          = 1;

    VkFenceCreateInfo fence_info = { 0 };
    fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Signaled so the very first begin_frame wait succeeds immediately.
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info          = { 0 };
    semaphore_info.sType                          = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    graphics_frame_t *POUND_RESTRICT frame_cursor = state->frames;

    for (uint32_t i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; ++i)
    {
        VkResult result = vkAllocateCommandBuffers(
            state->device, &allocate_info, &frame_cursor->command_buffer);

        if (result != VK_SUCCESS)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: vkAllocateCommandBuffers() returned %d for frame "
                            "%u.",
                            (int)result,
                            i);
            frame_cursor->command_buffer = NULL;
            state->status                = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }

        result = vkCreateFence(state->device, &fence_info, NULL, &frame_cursor->in_flight_fence);

        if (result != VK_SUCCESS)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: vkCreateFence() returned %d for frame %u.",
                            (int)result,
                            i);
            frame_cursor->in_flight_fence = NULL;
            state->status                 = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }

        result = vkCreateSemaphore(
            state->device, &semaphore_info, NULL, &frame_cursor->image_available_semaphore);

        if (result != VK_SUCCESS)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: vkCreateSemaphore() returned %d for frame %u.",
                            (int)result,
                            i);
            frame_cursor->image_available_semaphore = NULL;
            state->status                           = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }

        ++frame_cursor;
    }

    return state->status;
}

void
vulkan_frame_objects_destroy(graphics_state_t *POUND_RESTRICT state)
{
    if (NULL == state->device)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->device is NULL.");
        return;
    }

    graphics_frame_t *POUND_RESTRICT frames_cursor = state->frames;

    for (uint32_t i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; ++i)
    {
        if (frames_cursor->image_available_semaphore != NULL)
        {
            vkDestroySemaphore(state->device, frames_cursor->image_available_semaphore, NULL);
            frames_cursor->image_available_semaphore = NULL;
        }

        if (frames_cursor->in_flight_fence != NULL)
        {
            vkDestroyFence(state->device, frames_cursor->in_flight_fence, NULL);
            frames_cursor->in_flight_fence = NULL;
        }

        if (frames_cursor->command_buffer != NULL && state->command_pool != NULL)
        {
            vkFreeCommandBuffers(
                state->device, state->command_pool, 1, &frames_cursor->command_buffer);
            frames_cursor->command_buffer = NULL;
        }

        ++frames_cursor;
    }
}

POUND_COLD static bool
vulkan_is_graphics_layer_available(const char *POUND_RESTRICT layer_name)
{
    uint32_t count  = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, NULL);

    if (VK_SUCCESS != result)
    {
        POUND_LOG_ERROR(
            &thread_logger, "vkEnumerateInstanceLayerProperties() returned %d.", (int)result);
        return false;
    }

    if (0 == count)
    {
        false;
    }

    const memory_bucket_type_t previous_bucket
        = memory_subsystem_set_bucket(MEMORY_BUCKET_GRAPHICS);
    VkLayerProperties *POUND_RESTRICT layers
        = memory_subsystem_allocate(8U, (size_t)count * sizeof(VkLayerProperties));
    (void)memory_subsystem_set_bucket(previous_bucket);

    if (NULL == layers)
    {
        POUND_LOG_ERROR(&thread_logger, "Failed to allocate %u VkLayerProperties entries.", count);
        return false;
    }

    result = vkEnumerateInstanceLayerProperties(&count, layers);

    if (VK_SUCCESS != result)
    {
        POUND_LOG_ERROR(
            &thread_logger, "vkEnumerateInstanceLayerProperties returned %d.", (int)result);
        const memory_bucket_type_t bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_GRAPHICS);
        memory_subsystem_free(layers);
        (void)memory_subsystem_set_bucket(bucket);
        return false;
    }

    bool                                    found         = false;
    const VkLayerProperties *POUND_RESTRICT layers_cursor = layers;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (0 == strcmp(layers_cursor->layerName, layer_name))
        {
            found = true;
            break;
        }

        ++layers_cursor;
    }

    const memory_bucket_type_t bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_GRAPHICS);
    memory_subsystem_free(layers);
    (void)memory_subsystem_set_bucket(bucket);
    return found;
}

POUND_COLD static bool
vulkan_is_device_extension_supported(VkPhysicalDevice device, const char *POUND_RESTRICT name)
{
    uint32_t count  = 0U;
    VkResult result = vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(
            &thread_logger, "vkEnumerateDeviceExtensionProperties() returned %d.", (int)result);
        return false;
    }

    if (0 == count)
    {
        return false;
    }

    const memory_bucket_type_t previous_bucket
        = memory_subsystem_set_bucket(MEMORY_BUCKET_GRAPHICS);
    VkExtensionProperties *POUND_RESTRICT extensions
        = memory_subsystem_allocate(8U, (size_t)count * sizeof(VkExtensionProperties));
    (void)memory_subsystem_set_bucket(previous_bucket);

    if (NULL == extensions)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Failed to allocate %u VkExtensionProperties entries.", count);
        return false;
    }

    result = vkEnumerateDeviceExtensionProperties(device, NULL, &count, extensions);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(
            &thread_logger, "vkEnumerateDeviceExtensionProperties returned %d.", (int)result);
        const memory_bucket_type_t bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_GRAPHICS);
        memory_subsystem_free(extensions);
        (void)memory_subsystem_set_bucket(bucket);
        return false;
    }

    bool                                        found            = false;
    const VkExtensionProperties *POUND_RESTRICT extension_cursor = extensions;

    for (uint32_t i = 0U; i < count; ++i)
    {
        if (0 == strcmp(extension_cursor->extensionName, name))
        {
            found = true;
            break;
        }

        ++extension_cursor;
    }

    const memory_bucket_type_t bucket = memory_subsystem_set_bucket(MEMORY_BUCKET_GRAPHICS);
    memory_subsystem_free(extensions);
    (void)memory_subsystem_set_bucket(bucket);
    return found;
}

VkBool32
graphics_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                        VkDebugUtilsMessageTypeFlagsEXT             types,
                        const VkDebugUtilsMessengerCallbackDataEXT *data,
                        void                                       *user_data)
{
    (void)types;
    (void)user_data;

    if (NULL == data || NULL == data->pMessage)
    {
        return VK_FALSE;
    }

    if (0 != (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
    {
        POUND_LOG_ERROR(&thread_logger, "Vulkan validation: %s", data->pMessage);
    }
    else if (0 != (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
    {
        POUND_LOG_WARN(&thread_logger, "Vulkan validation: %s", data->pMessage);
    }
    else
    {
        POUND_LOG_DEBUG(&thread_logger, "Vulkan validation: %s", data->pMessage);
    }

    return VK_FALSE;
}