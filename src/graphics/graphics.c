#include "graphics.h"

#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_vulkan.h"
#include "log.h"
#include "memory/memory.h"

#define GRAPHICS_MAX_INSTANCE_EXTENSIONS 64
#define GRAPHICS_VALIDATION_LAYER_NAME   "VK_LAYER_KHRONOS_validation"

graphics_state_t g_graphics_state = { 0 };

POUND_COLD static pound_error_t sdl_window_create(graphics_state_t *POUND_RESTRICT        state,
                                                  const graphics_config_t *POUND_RESTRICT config);
POUND_COLD static pound_error_t vulkan_instance_create(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_debug_messenger_create(
    graphics_state_t *POUND_RESTRICT state);
POUND_COLD static pound_error_t vulkan_surface_create(graphics_state_t *POUND_RESTRICT state);
POUND_COLD static bool vulkan_is_graphics_layer_available(const char *POUND_RESTRICT layer_name);

static VKAPI_ATTR VkBool32 VKAPI_CALL
graphics_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                        VkDebugUtilsMessageTypeFlagsEXT             types,
                        const VkDebugUtilsMessengerCallbackDataEXT *data,
                        void                                       *user_data);

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
vulkan_instance_create(graphics_state_t *restrict state)
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
    const bool validation      = (1 != (state->state_flags & GRAPHICS_STATE_FLAG_VALIDATION));

    if (validation)
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
}

POUND_COLD static pound_error_t
vulkan_debug_messenger_create(graphics_state_t *restrict state)
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
vulkan_surface_create(graphics_state_t *restrict state)
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