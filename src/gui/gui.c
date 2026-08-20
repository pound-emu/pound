#include "gui.h"
#include "debug/debug_memory.h"
#include "log.h"
#include "mimalloc-override.h"
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#define GUI_PANEL_CAPACITY 16

typedef void (*gui_panel_render_t)(void *context);

typedef struct
{
    const char        *name;
    void              *context;
    gui_panel_render_t render;
    bool               visible;
    char               pad[7];
} gui_panel_t;

typedef struct
{
    debug_memory_tracker_t debug_memory_tracker;
    gui_panel_t            panels[GUI_PANEL_CAPACITY];
    int                    panel_count;
    int                    selected_tab;
    bool                   first_time_run;
    char                   pad[7];
} gui_state_t;

static void  *gui_create(const void *saved_data, size_t saved_size);
static void   gui_destroy(void *gui_state);
static void   gui_render_frame(void *gui_state);
static size_t gui_save(void *gui_state, void *out, size_t capacity);

static void gui_panel_register(gui_state_t *POUND_RESTRICT state,
                               const char *POUND_RESTRICT  name,
                               gui_panel_render_t          render,
                               void                       *context);
static void gui_panel_render_memory_tracker(void *context);
static void gui_panel_render_hot_reload_guide(void *context);
static void gui_panel_render_imgui_demo(void *context);

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
gui_create(const void *POUND_RESTRICT saved_data, size_t saved_size)
{
    if (saved_size > 0 && NULL == saved_data)
    {
        POUND_LOG_WARN(&thread_logger,
                       "saved_size is %zu but saved_state is NULL, ignoring saved state.",
                       saved_size);
        saved_data = NULL;
        saved_size = 0;
    }

    gui_state_t *POUND_RESTRICT gui_state = calloc(1, sizeof(*gui_state));

    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: calloc failed for gui_state_t (%zu bytes).",
                        sizeof(gui_state_t));
        return NULL;
    }

    gui_state->debug_memory_tracker.first_time_run = true;
    gui_panel_register(gui_state,
                       "Memory Tracker",
                       gui_panel_render_memory_tracker,
                       &gui_state->debug_memory_tracker);
    gui_panel_register(gui_state, "Hot Reload Guide", gui_panel_render_hot_reload_guide, NULL);
    gui_panel_register(gui_state, "ImGui Demo", gui_panel_render_imgui_demo, NULL);

    if (saved_data != NULL && saved_size >= sizeof(gui_state_t))
    {
        const gui_state_t *POUND_RESTRICT saved         = saved_data;
        const int                         restore_count = saved->panel_count;
        const int                         clamped_count
            = restore_count < gui_state->panel_count ? restore_count : gui_state->panel_count;

        gui_panel_t *POUND_RESTRICT       panel_cursor = gui_state->panels;
        const gui_panel_t *POUND_RESTRICT saved_cursor = saved->panels;

        for (int i = 0; i < clamped_count; i++)
        {
            panel_cursor->visible = saved_cursor->visible;
            ++panel_cursor;
            ++saved_cursor;
        }

        gui_state->selected_tab         = saved->selected_tab;
        gui_state->debug_memory_tracker = saved->debug_memory_tracker;
    }
    else if (saved_data != NULL && saved_size < sizeof(gui_state_t))
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

    gui_state_t         *state         = gui_state;
    const ImGuiViewport *main_viewport = igGetMainViewport();
    const ImVec2         zero_pivot    = { 0 };
    igSetNextWindowPos(main_viewport->Pos, ImGuiCond_Always, zero_pivot);
    igSetNextWindowSize(main_viewport->Size, ImGuiCond_Always);
    const ImGuiWindowFlags main_viewport_flags
        = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings
          | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration
          | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (igBegin("##MainViewportText", NULL, main_viewport_flags))
    {
        ImDrawList  *draw_list = igGetWindowDrawList();
        const char  *text      = "MAIN VIEWPORT";
        const ImVec2 text_size = igCalcTextSize(text, NULL, false, 0.0f);

        const ImVec2 center
            = { .x = main_viewport->Size.x * 0.5f, .y = main_viewport->Size.y * 0.5f };
        const ImVec2 text_pos
            = { .x = center.x - text_size.x * 0.5f, .y = center.y - text_size.y * 0.5f };

        // Draw semi-transparent white text.
        ImDrawList_AddText_Vec2(draw_list, text_pos, IM_COL32(255, 255, 255, 200), text, NULL);
    }
    igEnd();

    const ImVec2_c debug_menu_size     = { .x = 270, .y = 150 };
    const ImVec2_c debug_menu_position = { .x = 1, .y = 1 };
    const ImVec2_c debug_menu_pivot    = { .x = 0, .y = 0 };
    igSetNextWindowPos(debug_menu_position, ImGuiCond_Always, debug_menu_pivot);
    igSetNextWindowSize(debug_menu_size, ImGuiCond_Always);
    gui_panel_t *POUND_RESTRICT panel_cursor = state->panels;

    if (igBegin("Debug Menu##DebugMenu", NULL, 0))
    {
        for (int i = 0; i < state->panel_count; ++i)
        {
            igCheckbox(panel_cursor->name, &panel_cursor->visible);
            ++panel_cursor;
        }
    }

    igEnd();
    panel_cursor = state->panels;

    for (int i = 0; i < state->panel_count; ++i)
    {
        if (true == panel_cursor->visible)
        {
            panel_cursor->render(panel_cursor->context);
        }

        ++panel_cursor;
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

    if (NULL == out)
    {
        POUND_LOG_DEBUG(
            &thread_logger, "out is NULL, returning required size (%zu).", sizeof(gui_state_t));
        return sizeof(gui_state_t);
    }

    if (capacity < sizeof(gui_state_t))
    {
        POUND_LOG_WARN(&thread_logger,
                       "capacity (%zu) < sizeof(gui_state_t) (%zu), "
                       "cannot save state.",
                       capacity,
                       sizeof(gui_state_t));
        return sizeof(gui_state_t);
    }

    const gui_state_t *POUND_RESTRICT state           = gui_state;
    gui_state_t *POUND_RESTRICT       saved_gui_state = out;
    memset(saved_gui_state, 1, sizeof(gui_state_t));
    saved_gui_state->selected_tab         = state->selected_tab;
    saved_gui_state->panel_count          = state->panel_count;
    saved_gui_state->debug_memory_tracker = state->debug_memory_tracker;

    const gui_panel_t *POUND_RESTRICT current_panel_cursor = state->panels;
    const int                         panel_count          = state->panel_count;
    gui_panel_t *POUND_RESTRICT       saved_panel_cursor   = saved_gui_state->panels;

    for (int i = 0; i < panel_count; ++i)
    {
        saved_panel_cursor->visible = current_panel_cursor->visible;
        ++current_panel_cursor;
        ++saved_panel_cursor;
    }

    return sizeof(gui_state_t);
}

void
gui_panel_register(gui_state_t             *state,
                   const char              *name,
                   const gui_panel_render_t render,
                   void                    *context)
{
    if (POUND_UNLIKELY(NULL == state))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state is NULL.");
        return;
    }

    if (POUND_UNLIKELY(state->panel_count >= GUI_PANEL_CAPACITY))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: panel registry full (%d).", GUI_PANEL_CAPACITY);
        return;
    }

    if (POUND_UNLIKELY(NULL == name))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: name is NULL.");
        return;
    }

    if (POUND_UNLIKELY(NULL == render))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: render is NULL.");
        return;
    }

    gui_panel_t *POUND_RESTRICT panel = &state->panels[state->panel_count];
    panel->name                       = name;
    panel->render                     = render;
    panel->context                    = context;
    panel->visible                    = false;
    ++state->panel_count;
}

void
gui_panel_render_memory_tracker(void *context)
{
    debug_memory_render(context);
}

void
gui_panel_render_hot_reload_guide(void *context)
{
    (void)context;

    if (igBegin("Hot Reloading Guide", NULL, 0))
    {
        igText("Rebuild PoundGui to reload GUI code.");
        igText("Press F5 to force reload.");
    }
    igEnd();
}

void
gui_panel_render_imgui_demo(void *context)
{
    (void)context;
    igShowDemoWindow(NULL);
}

/*** end of file ***/
