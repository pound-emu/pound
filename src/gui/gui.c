#include "gui.h"
#include "debug/debug_memory.h"
#include "log.h"
#include "memory/memory.h"
#include "mimalloc-override.h"
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "gui_layout.h"

#include <cimgui.h>
#include <math.h>

#define GUI_PANEL_CAPACITY 16

typedef void (*gui_panel_render_t)(void *context);

typedef struct
{
    const char            *name;
    void                  *context;
    gui_panel_render_t     render;
    gui_layout_rectangle_t rectangle;
    /// Fraction of the screen space its region should occupy.
    float               fraction;
    gui_layout_region_t region;
    int                 layout_index;
    bool                visible;
    char                pad[3];
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

static gui_plugin_error_t gui_create(const void *saved_data, size_t saved_size, void **out);
static gui_plugin_error_t gui_destroy(void *gui_state);
static gui_plugin_error_t gui_render_frame(void *gui_state);
static gui_plugin_error_t gui_save(void   *gui_state,
                                   void   *out_gui,
                                   size_t  capacity,
                                   size_t *out_size);

static void gui_panel_register(gui_state_t *POUND_RESTRICT state,
                               const char *POUND_RESTRICT  name,
                               gui_panel_render_t          render,
                               void                       *context,
                               gui_layout_region_t         region,
                               float                       fraction);
static void gui_panel_render_memory_tracker(void *context);
static void gui_panel_render_hot_reload_guide(void *context);
static void gui_panel_render_imgui_demo(void *context);

gui_plugin_error_t
gui_plugin_exports_get(gui_plugin_exports_t *out)
{
    if (NULL == out)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out is NULL.");
        return GUI_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    out->create       = gui_create;
    out->destroy      = gui_destroy;
    out->render_frame = gui_render_frame;
    out->save         = gui_save;
    return true;
}

const char *
gui_plugin_error_to_string(const gui_plugin_error_t error)
{
    switch (error)
    {
        case GUI_PLUGIN_SUCCESS:
            return "a gui plugin operation was successful";
        case GUI_PLUGIN_ERROR_INVALID_ARGUMENT:
            return "a passed function argument was invalid";
        case GUI_PLUGIN_ERROR_ALLOCATION_FAILED:
            return "an allocator failed to allocate memory";
        case GUI_PLUGIN_ERROR_BUFFER_TOO_SMALL:
            return "a buffer was too small";
        case GUI_PLUGIN_ERROR_PANEL_REGISTRY_FULL:
            return "the panel registry is full";
    }
    return "UNKNOWN ERROR";
}

static gui_plugin_error_t
gui_create(const void *POUND_RESTRICT saved_data, size_t saved_size, void **out)
{
    if (NULL == out)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out is NULL.");
        return GUI_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    *out = NULL;

    if (saved_size > 0 && NULL == saved_data)
    {
        POUND_LOG_WARN(&thread_logger,
                       "saved_size is %zu but saved_state is NULL, ignoring saved state.",
                       saved_size);
        saved_data = NULL;
        saved_size = 0;
    }

    const size_t                memory_alignment = 8U;
    gui_state_t *POUND_RESTRICT gui_state
        = memory_subsystem_allocate(memory_alignment, sizeof(gui_state_t));
    (void)memset(gui_state, 0, sizeof(gui_state_t));

    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: calloc failed for gui_state_t (%zu bytes).",
                        sizeof(gui_state_t));
        return GUI_PLUGIN_ERROR_ALLOCATION_FAILED;
    }

    gui_state->debug_memory_tracker.first_time_run = true;

    gui_panel_register(gui_state,
                       "Memory Tracker",
                       gui_panel_render_memory_tracker,
                       &gui_state->debug_memory_tracker,
                       GUI_LAYOUT_REGION_RIGHT,
                       1.0F / 3.0F);
    gui_panel_register(gui_state,
                       "Hot Reload Guide",
                       gui_panel_render_hot_reload_guide,
                       NULL,
                       GUI_LAYOUT_REGION_RIGHT,
                       1.0F / 4.0F);
    gui_panel_register(
        gui_state, "ImGui Demo", gui_panel_render_imgui_demo, NULL, GUI_LAYOUT_REGION_NONE, 0.0F);

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

    *out = gui_state;
    return GUI_PLUGIN_SUCCESS;
}

static gui_plugin_error_t
gui_destroy(void *gui_state)
{
    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return GUI_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    memory_subsystem_free(gui_state);
    return GUI_PLUGIN_SUCCESS;
}

static gui_plugin_error_t
gui_render_frame(void *gui_state)
{
    if (POUND_UNLIKELY(NULL == gui_state))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return GUI_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    gui_state_t         *state         = gui_state;
    const ImGuiViewport *main_viewport = igGetMainViewport();

    if (POUND_UNLIKELY(NULL == main_viewport))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: main viewport is NULL.");
        return GUI_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    const float origin_x      = main_viewport->Pos.x;
    const float origin_y      = main_viewport->Pos.y;
    const float screen_width  = main_viewport->Size.x;
    const float screen_height = main_viewport->Size.y;

    int panel_count = state->panel_count;

    if (POUND_UNLIKELY(panel_count < 0))
    {
        panel_count = 0;
    }

    if (POUND_UNLIKELY(panel_count > 0))
    {
        panel_count = GUI_PANEL_CAPACITY;
    }

    // PASS 1: Register every visible docked panel with the layout.

    gui_layout_t                layout       = { 0 };
    gui_panel_t *POUND_RESTRICT panel_cursor = state->panels;

    for (int i = 0; i < panel_count; ++i)
    {
        panel_cursor->layout_index = -1;

        if ((true == panel_cursor->visible) && (GUI_LAYOUT_REGION_NONE != panel_cursor->region))
        {
            const int                layout_index = layout.panel_count;
            const gui_plugin_error_t error
                = gui_layout_add_panel(&layout, panel_cursor->region, panel_cursor->fraction);

            if (GUI_PLUGIN_SUCCESS == error)
            {
                panel_cursor->layout_index = layout_index;
            }
            else
            {
                POUND_LOG_WARN(&thread_logger,
                               "Panel '%s' will float this frame because %s.",
                               panel_cursor->name,
                               gui_plugin_error_to_string(error));
            }
        }

        ++panel_cursor;
    }

    gui_layout_rectangle_t   main_viewport_rectangle = { 0 };
    const gui_plugin_error_t error
        = gui_layout_compute(&layout, screen_width, screen_height, &main_viewport_rectangle);

    if (POUND_UNLIKELY(GUI_PLUGIN_SUCCESS != error))
    {
        POUND_LOG_WARN(&thread_logger,
                       "Falling back to full-screen viewport because %s.",
                       gui_plugin_error_to_string(error));
        main_viewport_rectangle.width  = screen_width;
        main_viewport_rectangle.height = screen_height;
    }

    const ImVec2 zero_pivot = { 0 };
    const ImVec2 main_viewport_position
        = { .x = origin_x + main_viewport_rectangle.x, .y = origin_y + main_viewport_rectangle.y };
    const ImVec2 viewport_size
        = { .x = main_viewport_rectangle.width, .y = main_viewport_rectangle.height };
    igSetNextWindowPos(main_viewport_position, ImGuiCond_Always, zero_pivot);
    igSetNextWindowSize(viewport_size, ImGuiCond_Always);

    const ImGuiWindowFlags main_viewport_flags
        = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings
          | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration
          | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (igBegin("##MainViewportText", NULL, main_viewport_flags))
    {
        ImDrawList  *draw_list       = igGetWindowDrawList();
        const char  *text            = "MAIN VIEWPORT";
        const ImVec2 text_size       = igCalcTextSize(text, NULL, false, 0.0f);
        const ImVec2 window_position = igGetWindowPos();
        const ImVec2 window_size     = igGetWindowSize();
        const ImVec2 center          = { .x = window_position.x + window_size.x * 0.5f,
                                         .y = window_position.y + window_size.y * 0.5f };
        const ImVec2 text_position
            = { .x = center.x - text_size.x * 0.5f, .y = center.y - text_size.y * 0.5f };

        // Draw semi-transparent white text.
        ImDrawList_AddText_Vec2(draw_list, text_position, IM_COL32(255, 255, 255, 200), text, NULL);
    }
    igEnd();

    // PASS 2:  Render each visible panel.

    panel_cursor = state->panels;

    for (int i = 0; i < panel_count; ++i)
    {
        if (true == panel_cursor->visible)
        {
            if ((panel_cursor->layout_index >= 0) && (true == layout.is_computed))
            {
                const gui_layout_rectangle_t rect = layout.panels[panel_cursor->layout_index];
                const ImVec2 panel_position = { .x = origin_x + rect.x, .y = origin_y + rect.y };
                const ImVec2 panel_size     = { .x = rect.width, .y = rect.height };
                igSetNextWindowPos(panel_position, ImGuiCond_Always, zero_pivot);
                igSetNextWindowSize(panel_size, ImGuiCond_Always);
            }

            panel_cursor->render(panel_cursor->context);
        }

        ++panel_cursor;
    }

    panel_cursor = state->panels;

    if (igBegin("Debug Menu##DebugMenu", NULL, 0))
    {
        for (int i = 0; i < state->panel_count; ++i)
        {
            igCheckbox(panel_cursor->name, &panel_cursor->visible);
            ++panel_cursor;
        }
    }

    igEnd();
    return GUI_PLUGIN_SUCCESS;
}

static gui_plugin_error_t
gui_save(void *gui_state, void *out_gui, size_t capacity, size_t *out_size)
{
    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return GUI_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == out_gui)
    {
        if (out_size != NULL)
        {
            *out_size = sizeof(gui_state_t);
        }

        POUND_LOG_DEBUG(
            &thread_logger, "out is NULL, returning required size (%zu).", sizeof(gui_state_t));
        return GUI_PLUGIN_SUCCESS;
    }

    if (capacity < sizeof(gui_state_t))
    {
        if (out_size != NULL)
        {
            *out_size = sizeof(gui_state_t);
        }

        POUND_LOG_WARN(&thread_logger,
                       "capacity (%zu) < sizeof(gui_state_t) (%zu), "
                       "cannot save state.",
                       capacity,
                       sizeof(gui_state_t));
        return GUI_PLUGIN_ERROR_BUFFER_TOO_SMALL;
    }

    const gui_state_t *POUND_RESTRICT state           = gui_state;
    gui_state_t *POUND_RESTRICT       saved_gui_state = out_gui;
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

    if (out_size != NULL)
    {
        *out_size = sizeof(gui_state_t);
    }

    return GUI_PLUGIN_SUCCESS;
}

static void
gui_panel_register(gui_state_t *POUND_RESTRICT state,
                   const char *POUND_RESTRICT  name,
                   gui_panel_render_t          render,
                   void                       *context,
                   gui_layout_region_t         region,
                   float                       fraction)
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

    if (POUND_UNLIKELY((region < GUI_LAYOUT_REGION_NONE) || (region > GUI_LAYOUT_REGION_BOTTOM)))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: region %d is invalid.", (int)region);
        return;
    }

    if (POUND_UNLIKELY((false == isfinite(fraction)) || (fraction < 0.0f) || (fraction > 1.0f)))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: fraction %f is outside [0, 1].", fraction);
        return;
    }

    gui_panel_t *POUND_RESTRICT panel = &state->panels[state->panel_count];
    panel->name                       = name;
    panel->render                     = render;
    panel->context                    = context;
    panel->region                     = region;
    panel->fraction                   = fraction;
    panel->layout_index               = -1;
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
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

    if (igBegin("Hot Reloading Guide", NULL, window_flags))
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
