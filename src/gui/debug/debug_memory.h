#ifndef POUND_MEMORY_DATA_H
#define POUND_MEMORY_DATA_H

#include "attributes.h"
#include <stdbool.h>
#include <stdint.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

typedef struct
{
    uint64_t current_frame;
    uint64_t gva_high;
    uint64_t gva_low;
    uint64_t gva_text_start;
    uint64_t gva_text_end;
    uint64_t gva_data_start;
    uint64_t gva_data_end;
    uint64_t gva_vram_framebuffer_start;
    uint64_t gva_vram_framebuffer_end;
    uint8_t  selected_box_info_index;
    bool     first_time_run;
    char     pad[6];
} debug_memory_tracker_t;

typedef struct
{
    // Text drawn inside the box. May be NULL.
    const char *label;

    /// Text drawn below the box. May be NULL.
    const char *caption;

    ImVec2_c position;
    float    width;
    float    height;
    ImU32    fill_color;

    char pad[4];
} debug_memory_gui_box_t;

typedef struct
{
    const char *name;
    const char *permissions;
    uint64_t    gva_start;
    uint64_t    gva_end;
} debug_memory_gui_box_info_t;

// debug_memory_data.c

POUND_HOT void debug_memory_gui_box_init(debug_memory_gui_box_t *POUND_RESTRICT box,
                                         ImVec2_c                               position,
                                         float                                  width,
                                         float                                  height,
                                         ImU32                                  fill_color,
                                         const char                            *label,
                                         const char                            *caption);

POUND_HOT ImU32 debug_memory_gui_box_lighten(ImU32 color, float amount);
bool            debug_memory_get_host_address_space_range(debug_memory_tracker_t *context);

// debug_memory_render.c

void debug_memory_render(debug_memory_tracker_t *context);

/// Renders `box` onto `draw_list`
///
/// Returns the bottom-right corner of the box.
POUND_HOT ImVec2_c debug_memory_gui_box_render(ImDrawList *POUND_RESTRICT draw_list,
                                               const debug_memory_gui_box_t *POUND_RESTRICT box);
POUND_HOT bool debug_memory_gui_box_is_clicked(const debug_memory_gui_box_t *POUND_RESTRICT box);
void           debug_memory_gui_box_info_render(const debug_memory_gui_box_info_t *info);

#endif // POUND_MEMORY_DATA_H

/*** end of file ***/