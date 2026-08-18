#include "mimalloc-override.h"

#include "gui.h"
#include "log.h"
#include "render/debug_memory_tracker.h"
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef struct
{
    int  selected_tab;
    bool show_demo;
    bool show_hot_reload_panel;
    bool show_mimalloc_panel;
    char pad[1];
} gui_state_t;

static void  *gui_create(const void *saved_data, size_t saved_size);
static void   gui_destroy(void *gui_state);
static void   gui_render_frame(void *gui_state);
static size_t gui_save(void *gui_state, void *out, size_t capacity);

bool
gui_exports_get(gui_exports_t *out)
{
    if (NULL == out)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out is NULL.");
        return false;
    }

    out->create       = gui_create;
    out->destroy      = gui_destroy;
    out->render_frame = gui_render_frame;
    out->save         = gui_save;
    return true;
}

static void *
gui_create(const void *POUND_RESTRICT saved_state, size_t saved_size)
{
    if (saved_size > 0 && NULL == saved_state)
    {
        POUND_LOG_WARN(&thread_logger,
                       "saved_size is %zu but saved_state is NULL, ignoring saved state.",
                       saved_size);
        saved_state = NULL;
        saved_size  = 0;
    }

    gui_state_t *POUND_RESTRICT gui_state = calloc(1, sizeof(*gui_state));

    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: calloc failed for gui_state_t (%zu bytes).",
                        sizeof(gui_state_t));
        return NULL;
    }

    gui_state->show_mimalloc_panel   = false;
    gui_state->show_demo             = false;
    gui_state->show_hot_reload_panel = false;

    if (saved_state && saved_size >= sizeof(*gui_state))
    {
        const gui_state_t *POUND_RESTRICT saved = saved_state;
        gui_state->show_mimalloc_panel          = saved->show_mimalloc_panel;
        gui_state->show_demo                    = saved->show_demo;
        gui_state->show_hot_reload_panel        = saved->show_hot_reload_panel;
        gui_state->selected_tab                 = saved->selected_tab;
    }
    else if (saved_state != NULL && saved_size < sizeof(*gui_state))
    {
        POUND_LOG_WARN(&thread_logger,
                       "saved_size (%zu) < sizeof(gui_state_t) (%zu), "
                       "using defaults.",
                       saved_size,
                       sizeof(gui_state_t));
    }
    else
    {
        POUND_LOG_DEBUG(&thread_logger, "No saved state, using defaults.");
    }

    return gui_state;
}

void
gui_destroy(void *gui_state)
{
    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return;
    }

    free(gui_state);
}

void
gui_render_frame(void *gui_state)
{
    if (POUND_UNLIKELY(NULL == gui_state))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return;
    }

    gui_state_t *state = gui_state;

    const ImVec2_c debug_menu_size     = { .x = 270, .y = 150 };
    const ImVec2_c debug_menu_position = { .x = 1, .y = 1 };
    const ImVec2_c debug_menu_pivot    = { .x = 0, .y = 0 };
    igSetNextWindowPos(debug_menu_position, ImGuiCond_Always, debug_menu_pivot);
    igSetNextWindowSize(debug_menu_size, ImGuiCond_Always);

    if (igBegin("Debug Menu", NULL, 0))
    {
        igCheckbox("Show Mimalloc Tracker", &state->show_mimalloc_panel);
        igCheckbox("Show Hot Reloading Guide", &state->show_hot_reload_panel);
        igCheckbox("Show ImGui Demo", &state->show_demo);
    }

    igEnd();

    if (state->show_mimalloc_panel)
    {
        gui_render_mimalloc_tracker();
    }

    if (state->show_hot_reload_panel)
    {
        igText("Rebuild PoundGui to relaod GUI code.");
        igText("Press F5 to force reload.");
    }

    if (state->show_demo)
    {
        igShowDemoWindow(NULL);
    }
}

size_t
gui_save(void *gui_state, void *out, size_t capacity)
{
    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return 0;
    }

    const gui_state_t *state = gui_state;

    if (NULL == out)
    {
        POUND_LOG_DEBUG(
            &thread_logger, "out is NULL, returning required size (%zu).", sizeof(*state));
        return sizeof(*state);
    }

    if (capacity < sizeof(*state))
    {
        POUND_LOG_WARN(&thread_logger,
                       "capacity (%zu) < sizeof(gui_state_t) (%zu), "
                       "cannot save state.",
                       capacity,
                       sizeof(*state));
        return sizeof(*state);
    }

    memcpy(out, state, sizeof(*state));
    return sizeof(*state);
}

/*** end of file ***/
