#include "graphics.h"

#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_vulkan.h"
#include "log.h"

graphics_state_t g_graphics_state = { 0 };

POUND_COLD static pound_error_t sdl_window_create(graphics_state_t *POUND_RESTRICT        state,
                                                  const graphics_config_t *POUND_RESTRICT config);

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
