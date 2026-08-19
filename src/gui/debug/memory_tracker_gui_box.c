#include "log.h"
#include "memory_tracker_gui_box.h"

#define GUI_BOX_LABEL_COLOR   IM_COL32(0, 0, 0, 255)
#define GUI_BOX_CAPTION_COLOR IM_COL32(128, 128, 128, 255)

/// How much brighter a box becomes while hovered (0.0 = unchanged, 1.0 = white).
#define GUI_BOX_HOVER_LIGHTEN 0.25F

static ImU32 gui_box_lighten(ImU32 color, float amount);

void
gui_box_init(gui_box_t     *box,
             const ImVec2_c position,
             const float    width,
             const float    height,
             const ImU32    fill_color,
             const char    *label,
             const char    *caption)
{
    if (POUND_UNLIKELY(NULL == box))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: box is NULL");
    }

    box->position   = position;
    box->width      = width;
    box->height     = height;
    box->fill_color = fill_color;
    box->label      = label;
    box->caption    = caption;
}

ImVec2_c
gui_box_render(ImDrawList *draw_list, const gui_box_t *box)
{
    ImVec2_c max_corner = { 0 };

    if (POUND_UNLIKELY(NULL == draw_list))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: draw_list is NULL.");
        return max_corner;
    }

    if (POUND_UNLIKELY(NULL == box))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: box is NULL.");
        return max_corner;
    }

    const float    font_height = igGetFontSize();
    const ImVec2_c min         = box->position;
    max_corner.x               = min.x + box->width;
    max_corner.y               = min.y + box->height;

    ImU32 fill_color = box->fill_color;

    if (igIsMouseHoveringRect(min, max_corner, true))
    {
        fill_color = gui_box_lighten(box->fill_color, GUI_BOX_HOVER_LIGHTEN);
    }

    const float       rounding = 0.0F;
    const ImDrawFlags flags    = 0;
    ImDrawList_AddRectFilled(draw_list, min, max_corner, fill_color, rounding, flags);

    if (NULL != box->label)
    {
        const ImVec2_c label_pos = { .x = min.x + GUI_BOX_LABEL_INSET,
                                     .y = max_corner.y - font_height - GUI_BOX_LABEL_INSET };
        ImDrawList_AddText_Vec2(draw_list, label_pos, GUI_BOX_LABEL_COLOR, box->label, NULL);
    }
    if (NULL != box->caption)
    {
        const ImVec2_c caption_pos = { .x = min.x, .y = max_corner.y + GUI_BOX_LABEL_INSET };
        ImDrawList_AddText_Vec2(draw_list, caption_pos, GUI_BOX_CAPTION_COLOR, box->caption, NULL);
    }

    return max_corner;
}

bool
gui_box_is_clicked(const gui_box_t *box)
{
    if (POUND_UNLIKELY(NULL == box))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: box is NULL.");
        return false;
    }

    const ImVec2_c min = box->position;
    const ImVec2_c max = { .x = min.x + box->width, .y = min.y + box->height };

    if (false == igIsMouseHoveringRect(min, max, true))
    {
        return false;
    }

    const bool is_mouse_clicked = igIsMouseClicked_Bool(ImGuiMouseButton_Left, false);
    return is_mouse_clicked;
}

void
gui_box_info_render(const gui_box_info_t *info)
{
    if (POUND_UNLIKELY(NULL == info))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: info is NULL.");
        return;
    }

    if (POUND_UNLIKELY(NULL == info->name))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: info->name is NULL.");
        return;
    }

    const ImVec4_c header_color
        = { .x = 100.0f / 255.0f, .y = 149.0f / 255.0f, .z = 237.0f / 255.0f, .w = 1.0f };
    const ImVec4_c label_color
        = { .x = 128.0f / 255.0f, .y = 128.0f / 255.0f, .z = 128.0f / 255.0f, .w = 1.0f };

    igTextColored(header_color, "Selected region");
    igSeparator();

    const ImVec2_c outer_size  = { .x = 0.0f, .y = 0.0f };
    const int      columns     = 2;
    const float    inner_width = 0.0F;

    if (false
        == igBeginTable("##gui_box_region_info",
                        columns,
                        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH,
                        outer_size,
                        inner_width))
    {
        return;
    }

    igTableNextRow(0, 0.0f);
    igTableSetColumnIndex(0);
    igTextColored(label_color, "name");
    igTableSetColumnIndex(1);
    igText("%s", info->name);

    igTableNextRow(0, 0.0f);
    igTableSetColumnIndex(0);
    igTextColored(label_color, "range");
    igTableSetColumnIndex(1);
    igText(
        "0x%llx - 0x%llx", (unsigned long long)info->gva_start, (unsigned long long)info->gva_end);
    igEndTable();
}

static ImU32
gui_box_lighten(const ImU32 color, const float amount)
{
    ImVec4_c rgba         = igColorConvertU32ToFloat4(color);
    rgba.x                = rgba.x + (1.0f - rgba.x) * amount;
    rgba.y                = rgba.y + (1.0f - rgba.y) * amount;
    rgba.z                = rgba.z + (1.0f - rgba.z) * amount;
    const ImU32 new_color = igColorConvertFloat4ToU32(rgba);
    return new_color;
}

/*** end of file ***/
