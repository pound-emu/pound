#include "debug_memory.h"
#include "log.h"
#include "memory/memory.h"
#include "mimalloc-stats.h"

/// How much brighter a box becomes while hovered (0.0 = unchanged, 1.0 = white).
#define GUI_BOX_HOVER_LIGHTEN 0.25F

#define GUI_BOX_LABEL_INSET   4.0F
#define GUI_BOX_LABEL_COLOR   IM_COL32(0, 0, 0, 255)
#define GUI_BOX_CAPTION_COLOR IM_COL32(128, 128, 128, 255)

POUND_HOT void progress_bar_render(double numerator_mib, double denominator_mib);

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
        context->selected_box_info_index = GUI_BOX_INDEX_UI;
        context->first_time_run          = false;

        memory_bucket_stats_t *POUND_RESTRICT bucket_stats
            = memory_subsystem_get_bucket_stats(MEMORY_BUCKET_UI);
        context->bucket_stats = bucket_stats;
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

    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

    if (true == igBegin(title, NULL, window_flags))
    {
        ImDrawList *POUND_RESTRICT draw_list   = igGetWindowDrawList();
        const ImVec2_c             origin      = igGetCursorScreenPos();
        const float                box_height  = 60.0f;
        const float                box_gap     = 4.0f;
        const float                font_height = igGetFontSize();
        const ImU32                box_color   = IM_COL32(100, 149, 237, 255);

        float       box_width       = 0.0F;
        const float available_width = igGetContentRegionAvail().x;
        ImVec2_c    box_positions[DEBUG_MEMORY_GUI_BOX_COUNT];

        debug_memory_gui_box_layout_compute(origin,
                                            available_width,
                                            box_gap,
                                            DEBUG_MEMORY_GUI_BOX_COUNT,
                                            &box_width,
                                            box_positions);

        const char *text_label_name          = ".text";
        const char *data_label_name          = ".data";
        const char *vram_label_name          = "VRAM";
        const char *ui_label_name            = "UI";
        const char *guest_memory_label_name  = "GUEST MEMORY";
        char        text_address[24]         = { 0 };
        char        data_address[24]         = { 0 };
        char        vram_address[24]         = { 0 };
        char        ui_address[24]           = "0x0";
        char        guest_memory_address[24] = "0x0";

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

        const char *ui_caption           = ui_address;
        const char *guest_memory_caption = guest_memory_address;

        const ImVec2_c ui_position           = box_positions[0];
        const ImVec2_c guest_memory_position = box_positions[1];
        const ImVec2_c text_position         = box_positions[2];
        const ImVec2_c data_position         = box_positions[3];
        const ImVec2_c vram_position         = box_positions[4];

        debug_memory_gui_box_t ui_box = { 0 };
        debug_memory_gui_box_init(
            &ui_box, ui_position, box_width, box_height, box_color, ui_label_name, ui_caption);
        debug_memory_gui_box_render(draw_list, &ui_box);

        debug_memory_gui_box_t guest_memory_box = { 0 };
        debug_memory_gui_box_init(&guest_memory_box,
                                  guest_memory_position,
                                  box_width,
                                  box_height,
                                  box_color,
                                  guest_memory_label_name,
                                  guest_memory_caption);
        debug_memory_gui_box_render(draw_list, &guest_memory_box);

        debug_memory_gui_box_t text_box = { 0 };
        debug_memory_gui_box_init(&text_box,
                                  text_position,
                                  box_width,
                                  box_height,
                                  box_color,
                                  text_label_name,
                                  text_caption);
        debug_memory_gui_box_render(draw_list, &text_box);

        debug_memory_gui_box_t data_box = { 0 };
        debug_memory_gui_box_init(&data_box,
                                  data_position,
                                  box_width,
                                  box_height,
                                  box_color,
                                  data_label_name,
                                  data_caption);
        debug_memory_gui_box_render(draw_list, &data_box);

        debug_memory_gui_box_t vram_box = { 0 };
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

        if (debug_memory_gui_box_is_clicked(&ui_box))
        {
            context->selected_box_info_index = GUI_BOX_INDEX_UI;
            context->bucket_stats            = memory_subsystem_get_bucket_stats(MEMORY_BUCKET_UI);
        }
        else if (debug_memory_gui_box_is_clicked(&guest_memory_box))
        {
            context->selected_box_info_index = GUI_BOX_INDEX_GUEST_MEMORY;
            context->bucket_stats = memory_subsystem_get_bucket_stats(MEMORY_BUCKET_GUEST_MEMORY);
        }
        else if (debug_memory_gui_box_is_clicked(&text_box))
        {
            context->selected_box_info_index = GUI_BOX_INDEX_TEXT;
        }
        else if (debug_memory_gui_box_is_clicked(&data_box))
        {
            context->selected_box_info_index = GUI_BOX_INDEX_DATA;
        }
        else if (debug_memory_gui_box_is_clicked(&vram_box))
        {
            context->selected_box_info_index = GUI_BOX_INDEX_VRAM;
        }
        else
        {
        }

        debug_memory_gui_box_info_t selected_box_info = { 0 };
        bool                        ignore_gva_range  = false;

        switch (context->selected_box_info_index)
        {
            case GUI_BOX_INDEX_GUEST_MEMORY: {
                selected_box_info.name        = guest_memory_label_name;
                selected_box_info.permissions = "R W";
                selected_box_info.gva_start   = 0x0U; // TODO: get gva addresses via jit_engine.h
                selected_box_info.gva_end     = 0x0U;
                break;
            }
            case GUI_BOX_INDEX_DATA: {
                selected_box_info.name        = data_label_name;
                selected_box_info.permissions = "R W";
                selected_box_info.gva_start   = context->gva_data_start;
                selected_box_info.gva_end     = context->gva_data_end;
                break;
            }
            case GUI_BOX_INDEX_VRAM: {
                selected_box_info.name        = vram_label_name;
                selected_box_info.permissions = "R W";
                selected_box_info.gva_start   = context->gva_vram_framebuffer_start;
                selected_box_info.gva_end     = context->gva_vram_framebuffer_end;
                break;
            }
            case GUI_BOX_INDEX_TEXT: {
                selected_box_info.name        = text_label_name;
                selected_box_info.permissions = "R X";
                selected_box_info.gva_start   = context->gva_text_start;
                selected_box_info.gva_end     = context->gva_text_end;
                break;
            }
            default: {
                selected_box_info.name        = ui_label_name;
                selected_box_info.permissions = "R W";
                selected_box_info.gva_start   = 0x0U; // Mimalloc gva ranges are not contiguous.
                selected_box_info.gva_end     = 0x0U;
                selected_box_info.used_bytes  = context->bucket_stats->used_bytes;
                ignore_gva_range              = true;
                break;
            }
        }

        debug_memory_gui_box_info_render(&selected_box_info, ignore_gva_range);

        const ImVec4_c header_color
            = { .x = 100.0f / 255.0f, .y = 149.0f / 255.0f, .z = 237.0f / 255.0f, .w = 1.0f };
        igTextColored(header_color, "Memory Utilization");

        const int    all_buckets      = -1;
        const size_t bytes_in_use     = memory_subsystem_get_bytes_used_by_bucket(all_buckets);
        const size_t heap_size        = memory_subsystem_get_heap_size();
        const double heap_size_mib    = (double)heap_size / (1024.0F * 1024.0F);
        const double bytes_in_use_mib = (double)bytes_in_use / (1024.0F * 1024.0F);

        igSeparator();
        igText("In Use vs Reserved");
        progress_bar_render(bytes_in_use_mib, heap_size_mib);
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
debug_memory_gui_box_info_render(const debug_memory_gui_box_info_t *info,
                                 const bool                         ignore_gva_range)
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

    igTableNextRow(0, 0.0f);
    igTableSetColumnIndex(0);
    igTextColored(label_color, "size");
    igTableSetColumnIndex(1);

    uint64_t size_bytes = 0;

    if (info->gva_start != UINT64_MAX && info->gva_end != 0 && info->gva_start < info->gva_end)
    {
        size_bytes = info->gva_end - info->gva_start;
    }

    if (true == ignore_gva_range)
    {
        size_bytes = info->used_bytes;
    }

    if (size_bytes != 0)
    {
        const double   size_mib = (double)size_bytes / (1024.0F * 1024.0F);
        const uint64_t pages    = (size_bytes / MEMORY_PAGE_SIZE)
                               + ((size_bytes % MEMORY_PAGE_SIZE) != 0ULL ? 1ULL : 0ULL);
        igText("%.2f MiB · %llu pages", size_mib, (unsigned long long)pages);
    }
    else
    {
        igText("Unknown");
    }

    igTableNextRow(0, 0.0f);
    igTableSetColumnIndex(0);
    igTextColored(label_color, "perms");
    igTableSetColumnIndex(1);
    igText("%s", info->permissions);

    igEndTable();
}

POUND_HOT void
progress_bar_render(const double numerator_mib, const double denominator_mib)
{
    float fraction = (denominator_mib > 0.0) ? (float)(numerator_mib / denominator_mib) : 0.0f;

    if (fraction < 0.0f)
    {
        fraction = 0.0f;
    }
    else if (fraction > 1.0f)
    {
        fraction = 1.0f;
    }

    char overlay[128];
    snprintf(overlay,
             sizeof(overlay),
             "%.1f / %.1f MiB (%.0f%%)",
             numerator_mib,
             denominator_mib,
             fraction * 100.0f);

    const ImVec2_c bar_size = { .x = -1.0f, .y = 0.0f };
    igProgressBar(fraction, bar_size, overlay);
}

/*** end of file ***/
