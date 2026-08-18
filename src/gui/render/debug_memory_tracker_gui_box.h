#ifndef POUND_DEBUG_MEMORY_TRACKER_GUI_BOX_H
#define POUND_DEBUG_MEMORY_TRACKER_GUI_BOX_H

#include "attributes.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#define GUI_BOX_LABEL_INSET 4.0F

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
} gui_box_t;

POUND_HOT void gui_box_init(gui_box_t *POUND_RESTRICT box,
                            ImVec2_c                  position,
                            float                     width,
                            float                     height,
                            ImU32                     fill_color,
                            const char               *label,
                            const char               *caption);

/// Renders `box` onto `draw_list`
///
/// Returns the bottom-right corner of the box.
POUND_HOT ImVec2_c gui_box_render(ImDrawList *POUND_RESTRICT      draw_list,
                                  const gui_box_t *POUND_RESTRICT box);

#endif // POUND_DEBUG_MEMORY_TRACKER_GUI_BOX_H

/*** end of file ***/