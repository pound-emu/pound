#include <mimalloc-override.h>

#include "gui/gui.h"
#include "log.h"
#include <SDL3/SDL.h>
#include <stdlib.h>

// This is required to perform hot reloading using Windows DLLs.
#if POUND_PLATFORM_WINDOWS

#define IMGL3W_IMPL

#endif // POUND_PLATFORM_WINDOWS

#include "imgui/backends/imgui_impl_opengl3_loader.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"

#define GUI_FREEZE_REASON_SIZE 256

typedef struct
{
    SDL_Window   *window;
    SDL_GLContext gl_context;
    ImGuiContext *imgui_context;
    bool          running;
    bool          gui_reload_request;
    bool          gui_frozen;
    char          gui_frozen_reason[GUI_FREEZE_REASON_SIZE];
    char          pad[5];
    char          gui_source_path[MAX_PATH];
    uint64_t      gui_source_time;
    uint64_t      gui_pending_time;
    uint64_t      gui_pending_since;
    uint64_t      gui_last_reload_attempt;
    uint64_t      gui_next_retry_ticks;
    gui_plugin_t  gui;
} app_t;

static bool app_video_init(app_t *app);
static bool app_video_shutdown(app_t *app);
static bool app_gui_init(app_t *app);
static bool app_gui_shutdown(app_t *app);
static bool app_gui_update(app_t *app, bool force);
static void app_gui_freeze(app_t *app, const char *reason);
static void app_gui_unfreeze(app_t *app);
static bool app_hot_reload_init(app_t *app);
static bool app_hot_reload_shutdown(app_t *app);
static void app_poll_events(app_t *app);
static void app_render_frame(app_t *app);
static void app_render_frozen_overlay(app_t *app);
static void app_memory_churn(void);

int
main(void)
{
    mi_option_set(mi_option_arena_reserve, 128 * 1024);
    pound_logger_init_default();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting process: Failed to initialise SDL because %s.",
                        SDL_GetError());
        return EXIT_FAILURE;
    }

    app_t app   = { 0 };
    app.running = true;

    if (false == app_video_init(&app))
    {
        app_video_shutdown(&app);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (false == app_gui_init(&app))
    {
        app_gui_shutdown(&app);
        app_video_shutdown(&app);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (false == app_hot_reload_init(&app))
    {
        POUND_LOG_WARN(&thread_logger, "Hot reloading is disabled. GUI will remain frozen.");
        app_gui_freeze(&app, "hot reload initialization failed");
    }

    const bool force_reload = true;

    if (false == app_gui_update(&app, force_reload))
    {
        POUND_LOG_WARN(
            &thread_logger,
            "Initial GUI load failed. GUI will remain frozen until hot reload succeeds.");
    }

    if (NULL == app.window)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting process: window is missing before first show.");
        app_hot_reload_shutdown(&app);
        app_gui_shutdown(&app);
        app_video_shutdown(&app);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_ShowWindow(app.window);

    if ((SDL_GetWindowFlags(app.window) & SDL_WINDOW_HIDDEN) != 0)
    {
        POUND_LOG_WARN(&thread_logger, "Window was shown but remains hidden, continuing anyway.");
    }

    POUND_LOG_INFO(&thread_logger, "Starting main loop.");

    while (app.running)
    {
        app_poll_events(&app);
        app_memory_churn();
        app_render_frame(&app);
    }

    app_gui_shutdown(&app);
    app_video_shutdown(&app);
    SDL_Quit();

    return EXIT_SUCCESS;
}

static bool
app_video_init(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    if (0 == SDL_WasInit(SDL_INIT_VIDEO))
    {
        POUND_LOG_WARN(&thread_logger,
                       "SDL video subsystem was not initialised. Attempting SDL_InitSubSystem.");

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            POUND_LOG_ERROR(
                &thread_logger,
                "Aborting function: failed to initialise SDL video subsystem because %s.",
                SDL_GetError());
            return false;
        }
    }

    POUND_LOG_DEBUG(&thread_logger, "Configuring video subsystem...");
    SDL_SetHint(SDL_HINT_APP_NAME, "Pound Emulator");

    if (!SDL_SetHint(SDL_HINT_APP_NAME, "Pound Emulator"))
    {
        POUND_LOG_WARN(
            &thread_logger, "Failed to set SDL application name because %s.", SDL_GetError());
    }

    const struct
    {
        SDL_GLAttr  attr;
        int         value;
        const char *name;
    } gl_attributes[] = {
        { .attr  = SDL_GL_CONTEXT_MAJOR_VERSION,
          .value = 3,
          .name  = "SDL_GL_CONTEXT_MAJOR_VERSION" },
        { .attr  = SDL_GL_CONTEXT_MINOR_VERSION,
          .value = 3,
          .name  = "SDL_GL_CONTEXT_MINOR_VERSION" },
        { .attr  = SDL_GL_CONTEXT_PROFILE_MASK,
          .value = SDL_GL_CONTEXT_PROFILE_CORE,
          .name  = "SDL_GL_CONTEXT_PROFILE_MASK" },
        { .attr  = SDL_GL_CONTEXT_FLAGS,
          .value = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG,
          .name  = "SDL_GL_CONTEXT_FLAGS" },
        { .attr = SDL_GL_DOUBLEBUFFER, .value = 1, .name = "SDL_GL_DOUBLEBUFFER" },
        { .attr = SDL_GL_DEPTH_SIZE, .value = 24, .name = "SDL_GL_DEPTH_SIZE" },
        { .attr = SDL_GL_STENCIL_SIZE, .value = 8, .name = "SDL_GL_STENCIL_SIZE" },
    };

    for (size_t i = 0; i < sizeof(gl_attributes) / sizeof(gl_attributes[0]); ++i)
    {
        if (!SDL_GL_SetAttribute(gl_attributes[i].attr, gl_attributes[i].value))
        {
            POUND_LOG_WARN(&thread_logger,
                           "Failed to set OpenGL attribute %s because %s.",
                           gl_attributes[i].name,
                           SDL_GetError());
        }
    }

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

    if (imgl3wInit() != GL3W_OK)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to initialise OpenGL loader (imgl3wInit).");
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
app_video_shutdown(app_t *app)
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

bool
app_gui_init(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    if (NULL == app->window)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: SDL window is NULL.");
        return false;
    }

    if (NULL == app->gl_context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: OpenGL context is NULL.");
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
        igDestroyContext(app->imgui_context);
        app->imgui_context = NULL;
        return false;
    }

    ImGuiIO *io = igGetIO_ContextPtr(app->imgui_context);

    if (NULL == io)
    {
        POUND_LOG_WARN(&thread_logger,
                       "ImGui IO is unavailable after initialisation, continuing with defaults.");
    }
    else
    {
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io->ConfigErrorRecovery             = true;
        io->ConfigErrorRecoveryEnableAssert = false;
    }

    igStyleColorsDark(NULL);
    POUND_LOG_INFO(&thread_logger, "Successfully configured GUI subsystem.");
    return true;
}

bool
app_gui_shutdown(app_t *app)
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
    gui_plugin_destroy(&app->gui);
    app->imgui_context = NULL;
    return true;
}

bool
app_gui_update(app_t *app, const bool force)
{
    if (POUND_UNLIKELY(NULL == app))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    if (POUND_UNLIKELY(0 == app->gui_source_path[0]))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: GUI plugin path is empty.");
        return false;
    }

    const uint64_t source_time = file_modified_time(app->gui_source_path);

    if (0 == source_time)
    {
        app->gui_pending_time = 0;

        if (true == app->gui.loaded)
        {
            return true;
        }

        app->gui_source_time = 0;
        return false;
    }

    if (false == force)
    {
        if (source_time == app->gui_source_time)
        {
            return app->gui.loaded;
        }

        // Delay reload so we do not load while the linker is working its magic.
        if (source_time != app->gui_pending_time)
        {
            app->gui_pending_time  = source_time;
            app->gui_pending_since = SDL_GetTicks();
            return app->gui.loaded;
        }

        if (SDL_GetTicks() - app->gui_pending_since < 150)
        {
            return app->gui.loaded;
        }
    }

    app->gui_pending_time = 0;

    // Save old plugin state.

    void  *hot_reloaded_code      = NULL;
    size_t hot_reloaded_code_size = 0;

    if (app->gui.loaded && app->gui.gui_handle && app->gui.exports.save)
    {
        hot_reloaded_code_size = app->gui.exports.save(app->gui.gui_handle, NULL, 0);

        if (hot_reloaded_code_size > 0)
        {
            hot_reloaded_code = malloc(hot_reloaded_code_size);

            if (hot_reloaded_code != NULL)
            {
                app->gui.exports.save(
                    app->gui.gui_handle, hot_reloaded_code, hot_reloaded_code_size);
            }
            else
            {
                hot_reloaded_code_size = 0;
            }
        }
    }

    // Save ImgGui layout.

    char       *ini_copy = NULL;
    const char *ini      = igSaveIniSettingsToMemory(NULL);

    if (ini != NULL)
    {
        const size_t ini_length = strlen(ini);
        ini_copy                = malloc(ini_length + 1);

        if (ini_copy != NULL)
        {
            memcpy(ini_copy, ini, ini_length);
            ini_copy[ini_length] = 0;
        }
    }

    bool         ok   = false;
    gui_plugin_t next = { 0 };

    if (true == gui_plugin_load_module(&next, app->gui_source_path))
    {
        void *next_handle = next.exports.create(hot_reloaded_code, hot_reloaded_code_size);

        if (next_handle != NULL)
        {
            gui_plugin_t old    = app->gui;
            app->gui            = next;
            app->gui.gui_handle = next_handle;
            app->gui.loaded     = true;

            gui_plugin_destroy(&old);

            if (ini_copy != NULL)
            {
                const size_t ini_size = strlen(ini_copy);
                igLoadIniSettingsFromMemory(ini_copy, ini_size);
                free(ini_copy);
                ini_copy = NULL;
            }

            app_gui_unfreeze(app);

            POUND_LOG_INFO(&thread_logger, "Loaded GUI plugin at %s", app->gui_source_path);
            ok = true;
        }
        else
        {
            POUND_LOG_ERROR(
                &thread_logger, "Failed to reload GUI plugin at %s", app->gui_source_path);
            gui_plugin_destroy(&next);
        }
    }
    else
    {
        POUND_LOG_ERROR(&thread_logger, "Failed to load GUI plugin at %s", app->gui_source_path);
    }

    free(hot_reloaded_code);
    free(ini_copy);
    app->gui_source_time = source_time;
    return ok;
}

void
app_gui_freeze(app_t *app, const char *reason)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return;
    }

    if (NULL == reason)
    {
        reason = "Unknown";
    }

    if (true == app->gui_frozen)
    {
        if (0 != strncmp(app->gui_frozen_reason, reason, sizeof(app->gui_frozen_reason)))
        {
            const int written
                = snprintf(app->gui_frozen_reason, sizeof(app->gui_frozen_reason), "%s", reason);

            if (written < 0 || (size_t)written >= sizeof(app->gui_frozen_reason))
            {
                app->gui_frozen_reason[sizeof(app->gui_frozen_reason) - 1] = '\0';
                POUND_LOG_WARN(&thread_logger, "GUI freeze reason was truncated.");
            }

            POUND_LOG_DEBUG(&thread_logger, "GUI freeze reason updated to %s", reason);
        }
        return;
    }

    POUND_LOG_ERROR(&thread_logger, "Freezing GUI because %s.", reason);
    app->gui_frozen = true;
    const int written
        = snprintf(app->gui_frozen_reason, sizeof(app->gui_frozen_reason), "%s", reason);

    if (written < 0 || (size_t)written >= sizeof(app->gui_frozen_reason))
    {
        app->gui_frozen_reason[sizeof(app->gui_frozen_reason) - 1] = '\0';
        POUND_LOG_WARN(&thread_logger, "GUI freeze reason was truncated.");
    }
}

void
app_gui_unfreeze(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return;
    }

    if (app->gui_frozen)
    {
        POUND_LOG_INFO(&thread_logger, "GUI hot reload succeeded; unfreezing GUI.");
    }

    app->gui_frozen           = false;
    app->gui_frozen_reason[0] = '\0';
}

bool
app_hot_reload_init(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    const char *base = SDL_GetBasePath();

    if (NULL == base)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: failed to get SDL base path.");
        return false;
    }

    const char  *separator   = "";
    const size_t base_length = strlen(base);

    if (base_length > 0 && base[base_length - 1] != '/' && base[base_length - 1] != '\\')
    {
        separator = "/";
    }

    snprintf(app->gui_source_path,
             sizeof(app->gui_source_path),
             "%s%s%s",
             base,
             separator,
             GUI_PLUGIN_NAME);

    app->gui_source_time   = file_modified_time(app->gui_source_path);
    app->gui_pending_time  = 0;
    app->gui_pending_since = 0;
    POUND_LOG_INFO(&thread_logger, "Found GUI plugin at %s", app->gui_source_path);
    POUND_LOG_DEBUG(&thread_logger, "Hot reloading is enabled.");
    return true;
}

bool
app_hot_reload_shutdown(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return false;
    }

    gui_plugin_destroy(&app->gui);
    memset(&app->gui, 0, sizeof(app->gui));

    app->gui_reload_request      = false;
    app->gui_frozen              = false;
    app->gui_source_time         = 0;
    app->gui_pending_time        = 0;
    app->gui_pending_since       = 0;
    app->gui_last_reload_attempt = 0;
    app->gui_next_retry_ticks    = 0;
    app->gui_source_path[0]      = '\0';
    app->gui_frozen_reason[0]    = '\0';

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

        switch (event.type)
        {
            case SDL_EVENT_QUIT: {
                POUND_LOG_INFO(&thread_logger, "Received SDL quit event.");
                app->running = false;
                break;
            }

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                if (event.window.windowID == SDL_GetWindowID(app->window))
                {
                    POUND_LOG_INFO(&thread_logger, "Received window close request.");
                    app->running = false;
                }
                break;
            }

            case SDL_EVENT_KEY_DOWN: {
                if (SDL_SCANCODE_F5 == event.key.scancode)
                {
                    POUND_LOG_DEBUG(&thread_logger, "F5 pressed; GUI reload requested.");
                    app->gui_reload_request = true;
                }
                break;
            }

            default: {
                break;
            }
        }
    }
}

void
app_render_frame(app_t *app)
{
    if (POUND_UNLIKELY(NULL == app))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return;
    }

    if (POUND_UNLIKELY(NULL == app->window))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: SDL window is NULL.");
        app->running = false;
        return;
    }

    if (POUND_UNLIKELY(NULL == app->gl_context))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: OpenGL context is NULL.");
        app->running = false;
        return;
    }

    if (POUND_UNLIKELY(false == app_gui_update(app, app->gui_reload_request)))
    {
        if (false == app->gui_frozen)
        {
            app_gui_freeze(app, "GUI update failed.");
        }
    }

    app->gui_reload_request = false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();

    if (app->gui_frozen)
    {
        app_render_frozen_overlay(app);
    }
    else if (app->gui.loaded && app->gui.gui_handle && app->gui.exports.render_frame)
    {
        app->gui.exports.render_frame(app->gui.gui_handle);
    }
    else
    {
        app_gui_freeze(app, "GUI plugin state became invalid.");
        app_render_frozen_overlay(app);
    }

    igRender();

    glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    SDL_GL_SwapWindow(app->window);
}

void
app_render_frozen_overlay(app_t *app)
{
    if (NULL == app)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: app context is NULL.");
        return;
    }

    if (NULL == app->imgui_context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: ImGui context is NULL.");
        return;
    }

    ImGuiIO *io = igGetIO_ContextPtr(app->imgui_context);

    if (NULL == io)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "ImGui IO is unavailable; cannot render frozen GUI overlay.");
        return;
    }

    ImVec2 zero;
    zero.x = 0.0f;
    zero.y = 0.0f;

    igSetNextWindowPos(zero, ImGuiCond_Always, zero);
    igSetNextWindowSize(io->DisplaySize, ImGuiCond_Always);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                                   | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
                                   | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    if (igBegin("Pound GUI Frozen", NULL, flags))
    {
        igText("GUI is frozen due to a hot reload failure.");
        igSeparator();

        igText("Source plugin:");
        igTextWrapped("%s", app->gui_source_path[0] != '\0' ? app->gui_source_path : "<unknown>");
        igSeparator();

        igText("Reason:");
        igTextWrapped("%s", app->gui_frozen_reason[0] != '\0' ? app->gui_frozen_reason : "Unknown");
        igSeparator();

        igText("The GUI will remain frozen until a hot reload succeeds.");
        igText("Press F5 to force a reload.");
    }

    igEnd();
}

// Slowly allocate and deallocate memory over 10 seconds.
// This is meant to test the GUI memory debug tracker.
static void
app_memory_churn(void)
{
    enum
    {
        BLOCK_MAX = 128
    };

    static void    *blocks[BLOCK_MAX] = { 0 };
    static size_t   count             = 0;
    static uint64_t start_ms          = 0;

    const size_t   block_size  = 256 * 1024;
    const uint64_t duration_ms = 10000;
    const uint64_t half_ms     = duration_ms / 2;
    const size_t   max_step    = 4;

    const uint64_t now = SDL_GetTicks();

    if (0 == start_ms)
    {
        start_ms = now;
    }

    const uint64_t elapsed = (now - start_ms) % duration_ms;

    size_t target;

    if (elapsed < half_ms)
    {
        target = (size_t)((elapsed * BLOCK_MAX) / half_ms);
    }
    else
    {
        target = BLOCK_MAX - (size_t)(((elapsed - half_ms) * BLOCK_MAX) / half_ms);
    }

    if (target > BLOCK_MAX)
    {
        target = BLOCK_MAX;
    }

    size_t step = 0;

    while (count < target && step < max_step)
    {
        void *p = malloc(block_size);

        if (NULL == p)
        {
            break;
        }

        memset(p, 0xCD, block_size);
        blocks[count++] = p;
        ++step;
    }

    while (count > target && step < max_step)
    {
        free(blocks[--count]);
        blocks[count] = NULL;
        ++step;
    }
}

/*** end of file ***/
