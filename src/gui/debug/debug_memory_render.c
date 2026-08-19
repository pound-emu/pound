#include "debug_memory.h"
#include "log.h"
#include "mimalloc-stats.h"

/// How much brighter a box becomes while hovered (0.0 = unchanged, 1.0 = white).
#define GUI_BOX_HOVER_LIGHTEN 0.25F

#define GUI_BOX_LABEL_INSET   4.0F
#define GUI_BOX_LABEL_COLOR   IM_COL32(0, 0, 0, 255)
#define GUI_BOX_CAPTION_COLOR IM_COL32(128, 128, 128, 255)

void
debug_memory_render(debug_memory_tracker_t *context)
{
    if (POUND_UNLIKELY(NULL == context))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: context is NULL.");
        return;
    }

    if (POUND_UNLIKELY(true == context->first_time_run))
    {
        debug_memory_get_host_address_space_range(context);
        context->selected_box_info_index = 0;
        context->first_time_run          = false;
    }

    if (0 == (++context->current_frame & 127))
    {
        debug_memory_get_host_address_space_range(context);
    }

    char title[160];

    if (POUND_LIKELY(0 != context->gva_high))
    {
        snprintf(title,
                 sizeof(title),
                 "Guest Address Space - 0x%llx - 0x%llx (mapped view)###GuestAddressSpace",
                 (unsigned long long)context->gva_low,
                 (unsigned long long)context->gva_high);
    }
    else
    {
        snprintf(title, sizeof(title), "Guest Address Space - unknown###GuestAddressSpace");
    }

    mi_stats_t_decl(stats);

    if (true == igBegin(title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImDrawList *POUND_RESTRICT draw_list   = igGetWindowDrawList();
        const ImVec2_c             origin      = igGetCursorScreenPos();
        const float                box_width   = 150.0f;
        const float                box_height  = 60.0f;
        const float                box_gap     = 4.0f;
        const float                font_height = igGetFontSize();
        const ImU32                box_color   = IM_COL32(100, 149, 237, 255);

        const char *text_label_name  = ".text";
        const char *data_label_name  = ".data";
        const char *vram_label_name  = "VRAM";
        char        text_address[24] = { 0 };
        char        data_address[24] = { 0 };
        char        vram_address[24] = { 0 };

        int written = snprintf(text_address,
                               sizeof(text_address),
                               "0x%llx",
                               (unsigned long long)context->gva_text_start);

        const char *text_caption
            = (written > 0 && written < (int)sizeof(text_address)) ? text_address : NULL;

        written = snprintf(data_address,
                           sizeof(data_address),
                           "0x%llx",
                           (unsigned long long)context->gva_data_start);

        const char *data_caption
            = (written > 0 && written < (int)sizeof(data_address)) ? data_address : NULL;

        written = snprintf(vram_address,
                           sizeof(vram_address),
                           "0x%llx",
                           (unsigned long long)context->gva_vram_framebuffer_start);

        const char *vram_caption
            = (written > 0 && written < (int)sizeof(vram_address)) ? vram_address : NULL;

        debug_memory_gui_box_t text_box = { 0 };
        debug_memory_gui_box_init(
            &text_box, origin, box_width, box_height, box_color, text_label_name, text_caption);
        const ImVec2_c text_box_end = debug_memory_gui_box_render(draw_list, &text_box);

        debug_memory_gui_box_t data_box;
        const ImVec2_c         data_position = { .x = text_box_end.x + box_gap, .y = origin.y };
        debug_memory_gui_box_init(&data_box,
                                  data_position,
                                  box_width,
                                  box_height,
                                  box_color,
                                  data_label_name,
                                  data_caption);
        const ImVec2_c data_box_end = debug_memory_gui_box_render(draw_list, &data_box);

        debug_memory_gui_box_t vram_box;
        const ImVec2_c         vram_position = { .x = data_box_end.x + box_gap, .y = origin.y };
        debug_memory_gui_box_init(&vram_box,
                                  vram_position,
                                  box_width,
                                  box_height,
                                  box_color,
                                  vram_label_name,
                                  vram_caption);
        debug_memory_gui_box_render(draw_list, &vram_box);

        const ImVec2_c reserved
            = { .x = 0.0f,
                .y = box_height + GUI_BOX_LABEL_INSET + font_height + GUI_BOX_LABEL_INSET };
        igDummy(reserved);

        if (debug_memory_gui_box_is_clicked(&text_box))
        {
            context->selected_box_info_index = 0;
        }
        else if (debug_memory_gui_box_is_clicked(&data_box))
        {
            context->selected_box_info_index = 1;
        }
        else if (debug_memory_gui_box_is_clicked(&vram_box))
        {
            context->selected_box_info_index = 2;
        }
        else
        {
        }

        const int32_t               selected_index    = context->selected_box_info_index;
        debug_memory_gui_box_info_t selected_box_info = { 0 };

        switch (selected_index)
        {
            case 1: {
                selected_box_info.name      = data_label_name;
                selected_box_info.gva_start = context->gva_data_start;
                selected_box_info.gva_end   = context->gva_data_end;
                break;
            }
            case 2: {
                selected_box_info.name      = vram_label_name;
                selected_box_info.gva_start = context->gva_vram_framebuffer_start;
                selected_box_info.gva_end   = context->gva_vram_framebuffer_end;
                break;
            }
            default: {
                selected_box_info.name      = text_label_name;
                selected_box_info.gva_start = context->gva_text_start;
                selected_box_info.gva_end   = context->gva_text_end;
                break;
            }
        }

        debug_memory_gui_box_info_render(&selected_box_info);

        if (mi_stats_get(&stats))
        {
            const double memory_in_use
                = (double)(stats.malloc_normal.current + stats.malloc_huge.current)
                  / (1024.0 * 1024.0);

            igSeparator();
            igText(
                "reserved: %.1f MiB   committed: %.1f MiB   in use: %.1f MiB   malloc req: %.1f "
                "MiB",
                (double)stats.reserved.current / (1024.0 * 1024.0),
                (double)stats.committed.current / (1024.0 * 1024.0),
                memory_in_use,
                (double)stats.malloc_requested.total / (1024.0 * 1024.0));
        }
    }

    igEnd();
}

ImVec2_c
debug_memory_gui_box_render(ImDrawList *draw_list, const debug_memory_gui_box_t *box)
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
        fill_color = debug_memory_gui_box_lighten(box->fill_color, GUI_BOX_HOVER_LIGHTEN);
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
debug_memory_gui_box_is_clicked(const debug_memory_gui_box_t *box)
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
debug_memory_gui_box_info_render(const debug_memory_gui_box_info_t *info)
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

/*** end of file ***/
