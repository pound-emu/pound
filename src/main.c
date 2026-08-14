#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "mimalloc-override.h"

#include "log.h"

#include "SDL3/SDL.h"
#include "cimgui.h"
#include "cimgui_impl.h"
#include <stdlib.h>

typedef struct
{
    SDL_Window   *window;
    SDL_GLContext gl_context;
    ImGuiContext *imgui_context;
    bool          running;
    char          pad[7];
} app_t;

static bool app_init_video(app_t *app);
static bool app_init_ui(app_t *app);
static bool app_shutdown_ui(app_t *app);
static bool app_shutdown_video(app_t *app);
static void app_poll_events(app_t *app);
static void app_render_frame(app_t *app);

int
main(void)
{
    mi_option_set(mi_option_arena_reserve, 128 * 1024);
    pound_logger_init_default();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting process: Failed to initialise SDL because %s",
                        SDL_GetError());
        return EXIT_FAILURE;
    }

    app_t app   = { 0 };
    app.running = true;

    if (false == app_init_video(&app))
    {
        app_shutdown_video(&app);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (false == app_init_ui(&app))
    {
        app_shutdown_ui(&app);
        app_shutdown_video(&app);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_ShowWindow(app.window);

    while (app.running)
    {
        app_poll_events(&app);
        app_render_frame(&app);
    }

    app_shutdown_ui(&app);
    app_shutdown_video(&app);
    SDL_Quit();

    return EXIT_SUCCESS;
}

static bool
app_init_video(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    POUND_LOG_DEBUG(&thread_logger, "Configuring video subsystem...");
    SDL_SetHint(SDL_HINT_APP_NAME, "Pound Emulator");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    app->window = SDL_CreateWindow("Pound Emulator",
                                   1270,
                                   720,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
                                       | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);

    if (NULL == app->window)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to create window because %s.",
                        SDL_GetError());
        return false;
    }

    app->gl_context = SDL_GL_CreateContext(app->window);

    if (NULL == app->gl_context)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to create OpenGL context because %s.",
                        SDL_GetError());
        return false;
    }

    if (false == SDL_GL_MakeCurrent(app->window, app->gl_context))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to make OpenGL context current because %s.",
                        SDL_GetError());
        return false;
    }

    if (false == SDL_GL_SetSwapInterval(1))
    {
        POUND_LOG_WARN(&thread_logger,
                       "Aborting function: failed to set swap interval because %s.",
                       SDL_GetError());
    }

    POUND_LOG_INFO(&thread_logger, "Successfully configured video subsystem.");
    return true;
}

bool
app_init_ui(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    POUND_LOG_DEBUG(&thread_logger, "Configuring UI subsystem...");
    app->imgui_context = igCreateContext(NULL);

    if (NULL == app->imgui_context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: failed to create ImGui context.");
        return false;
    }

    if (false == ImGui_ImplSDL3_InitForOpenGL(app->window, app->gl_context))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to initialize ImGui SDL3 backend.");
        igDestroyContext(app->imgui_context);
        app->imgui_context = NULL;
        return false;
    }

    if (false == ImGui_ImplOpenGL3_Init("#version 330"))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: failed to initialize OpenGL3 backend.");
        ImGui_ImplOpenGL3_Shutdown();
        app->imgui_context = NULL;
        return false;
    }

    igStyleColorsDark(NULL);
    POUND_LOG_INFO(&thread_logger, "Successfully configured UI subsystem.");
    return true;
}

bool
app_shutdown_ui(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    if (NULL == app->imgui_context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: ImGui context is NULL.");
        return false;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(app->imgui_context);
    app->imgui_context = NULL;
    return true;
}

bool
app_shutdown_video(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    if (NULL == app->gl_context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: OpenGL context is NULL.");
        return false;
    }

    SDL_GL_MakeCurrent(app->window, NULL);
    SDL_GL_DestroyContext(app->gl_context);
    app->gl_context = NULL;

    if (NULL == app->window)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: SDL window is NULL.");
        return false;
    }

    SDL_DestroyWindow(app->window);
    app->window = NULL;
    return true;
}

void
app_poll_events(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return;
    }

    if (NULL == app->window)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: SDL window is NULL.");
        return;
    }

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (SDL_EVENT_QUIT == event.type)
        {
            app->running = false;
        }

        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
            && event.window.windowID == SDL_GetWindowID(app->window))
        {
            app->running = false;
        }
    }
}

void
app_render_frame(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();

    igShowDemoWindow(NULL);

    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    SDL_GL_SwapWindow(app->window);
}

/*** end of file ***/
