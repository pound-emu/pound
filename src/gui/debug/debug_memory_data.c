#include "debug_memory.h"
#include "log.h"
#include "mimalloc-stats.h"
#include "platform.h"
#include <errno.h>
#include <stdlib.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#if POUND_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif POUND_PLATFORM_POSIX

#define __USE_XOPEN_EXTENDED
#include <unistd.h>

#endif // POUND_PLATFORM_WINDOWS

#define TRACKER_PATH_CAPACITY 4096
#define TRACKER_LINE_CAPACITY 1024

bool
debug_memory_get_host_address_space_range(debug_memory_tracker_t *context)
{
    if (NULL == context)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: context is NULL.");
        return false;
    }

    uint64_t low                    = UINT64_MAX;
    uint64_t high                   = 0;
    uint64_t text_start             = UINT64_MAX;
    uint64_t text_end               = 0;
    uint64_t data_start             = UINT64_MAX;
    uint64_t data_end               = 0;
    uint64_t vram_framebuffer_start = UINT64_MAX;
    uint64_t vram_framebuffer_end   = 0;

#if POUND_PLATFORM_WINDOWS

    const HMODULE self_module = GetModuleHandleW(NULL);

    if (NULL == self_module)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: GetModuleHandleW(NULL) failed (Win32 error %lu).",
                        (unsigned long)GetLastError());
        return false;
    }

    uint64_t       matched_regions = 0;
    const uint8_t *address         = NULL;

    for (;;)
    {
        MEMORY_BASIC_INFORMATION info;
        const SIZE_T             queried = VirtualQuery(address, &info, sizeof(info));

        if (0 == queried)
        {
            if (NULL == address)
            {
                POUND_LOG_ERROR(
                    &thread_logger,
                    "Aborting function: VirtualQuery failed at address 0 (Win32 error %lu).",
                    (unsigned long)GetLastError());
                return false;
            }

            break;
        }

        if (queried < sizeof(info))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: VirtualQuery returned %zu bytes, expected %zu.",
                            (size_t)queried,
                            sizeof(info));
            return false;
        }

        const uint8_t *const region_start = (const uint8_t *)info.BaseAddress;
        const uint8_t       *region_end   = region_start + info.RegionSize;

        if (region_end < region_start)
        {
            POUND_LOG_WARN(&thread_logger, "Stopping VirtualQuery walk: region end overflowed.");
            break;
        }

        const bool is_free    = (MEM_FREE == info.State);
        const bool is_private = (MEM_PRIVATE == info.Type);
        const bool is_self    = (info.AllocationBase == (PVOID)self_module);

        if (false == is_free && (is_private || is_self))
        {
            const uint64_t start_hex = (uint64_t)(uintptr_t)region_start;
            const uint64_t end_hex   = (uint64_t)(uintptr_t)region_end;

            if (start_hex < low)
            {
                low = start_hex;
            }

            if (end_hex > high)
            {
                high = end_hex;
            }

            ++matched_regions;
        }

        if (region_end <= address)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: VirtualQuery region did not advance.");
            return false;
        }

        address = region_end;
    }

    if (0 == matched_regions)
    {
        POUND_LOG_WARN(&thread_logger, "No host program regions found via VirtualQuery.");
        return false;
    }

#elif POUND_PLATFORM_POSIX

    char          exe_path[TRACKER_PATH_CAPACITY] = { 0 };
    const ssize_t exe_length = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

    if (exe_length < 0)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: readlink(\"/proc/self/exe\") failed because %s.",
                        strerror(errno));
        return false;
    }

    if (0 == exe_length)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: readlink returned an empty path.");
        return false;
    }

    if ((size_t)exe_length >= sizeof(exe_path))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: executable path exceeds %zu bytes.",
                        sizeof(exe_path) - 1);
        return false;
    }

    exe_path[exe_length]      = '\0';
    FILE *POUND_RESTRICT maps = fopen("/proc/self/maps", "r");

    if (NULL == maps)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: fopen(/proc/self/maps, r) failed because %s.",
                        strerror(errno));
        return false;
    }

    uint64_t matched_regions = 0;
    uint64_t line_number     = 0;
    char     line[TRACKER_LINE_CAPACITY];

    while (fgets(line, sizeof(line), maps))
    {
        ++line_number;

        if (NULL == strchr(line, '\n') && !feof(maps))
        {
            POUND_LOG_WARN(&thread_logger,
                           "/proc/self/maps line %llu exceeds %zu bytes, skipping.",
                           (unsigned long long)line_number,
                           sizeof(line) - 1);
            char drain[256];

            while (fgets(drain, sizeof(drain), maps) && NULL == strchr(drain, '\n'))
            {
            }

            continue;
        }

        char              *line_cursor = line;
        char              *next        = NULL;
        unsigned long long start_hex   = 0;
        unsigned long long end_hex     = 0;
        errno                          = 0;
        start_hex                      = strtoull(line_cursor, &next, 16);

        if (next == line_cursor || ERANGE == errno)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: invalid start address.",
                           (unsigned long long)line_number);
            continue;
        }

        if ('-' != *next)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: missing '-' separator.",
                           (unsigned long long)line_number);
            continue;
        }

        errno       = 0;
        line_cursor = next + 1;
        end_hex     = strtoull(line_cursor, &next, 16);

        if (next == line_cursor || ERANGE == errno)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: invalid end address.",
                           (unsigned long long)line_number);
            continue;
        }

        line_cursor = next;

        // perms (single non-space token)
        if (' ' != *line_cursor)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: malformed permissions field.",
                           (unsigned long long)line_number);
            continue;
        }

        ++line_cursor;
        char permissions[5] = { 0 };

        for (int p = 0; p < 4 && '\0' != *line_cursor && ' ' != *line_cursor; ++p, ++line_cursor)
        {
            permissions[p] = *line_cursor;
        }

        if ('\0' != *line_cursor && ' ' != *line_cursor)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: permissions field too long.",
                           (unsigned long long)line_number);
            continue;
        }

        // offset (hex)
        if (' ' != *line_cursor)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: malformed offset field.",
                           (unsigned long long)line_number);
            continue;
        }

        ++line_cursor;
        errno = 0;
        (void)strtoull(line_cursor, &next, 16);

        if (next == line_cursor || ERANGE == errno)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: invalid offset field.",
                           (unsigned long long)line_number);
            continue;
        }

        line_cursor = next;

        // dev (hex major ':' hex minor)
        if (' ' != *line_cursor)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: malformed device field.",
                           (unsigned long long)line_number);
            continue;
        }

        ++line_cursor;
        errno = 0;
        (void)strtoull(line_cursor, &next, 16);

        if (next == line_cursor || ERANGE == errno || ':' != *next)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: invalid device major.",
                           (unsigned long long)line_number);
            continue;
        }

        line_cursor = next + 1;
        errno       = 0;
        (void)strtoull(line_cursor, &next, 16);

        if (next == line_cursor || ERANGE == errno)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: invalid device minor.",
                           (unsigned long long)line_number);
            continue;
        }

        line_cursor = next;

        // inode (decimal)
        if (' ' != *line_cursor)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: malformed inode field.",
                           (unsigned long long)line_number);
            continue;
        }

        ++line_cursor;
        errno = 0;
        (void)strtoull(line_cursor, &next, 10);

        if (next == line_cursor || ERANGE == errno)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: invalid inode field.",
                           (unsigned long long)line_number);
            continue;
        }

        line_cursor = next;

        // pathname: skip leading spaces, strip line terminator.
        while (' ' == *line_cursor)
        {
            ++line_cursor;
        }

        char *path                  = line_cursor;
        path[strcspn(path, "\r\n")] = '\0';

        if (end_hex < start_hex)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: end < start.",
                           (unsigned long long)line_number);
            continue;
        }

        const bool is_anonymous = ('\0' == path[0]);
        const bool is_heap      = (0 == strcmp(path, "[heap]"));
        const bool is_stack     = (0 == strncmp(path, "[stack", 6));
        const bool is_device    = (0 == strncmp(path, "/dev/", 5)); // VRAM/GPU mappings
        const bool is_self      = (0 == strcmp(path, exe_path));    // our code/rodata/data

        if (is_anonymous || is_heap || is_stack || is_device || is_self)
        {
            if (start_hex < low)
            {
                low = start_hex;
            }

            if (end_hex > high)
            {
                high = end_hex;
            }

            ++matched_regions;
        }

        if (true == is_self && 'r' == permissions[0] && '-' == permissions[1]
            && 'x' == permissions[2])
        {
            text_start = start_hex;
            text_end   = end_hex;
        }
        else if (true == is_self && 'r' == permissions[0] && 'w' == permissions[1]
                 && '-' == permissions[2])
        {
            data_start = start_hex;
            data_end   = end_hex;
        }
        else
        {
        }

        if (true == is_device && 'w' == permissions[1])
        {
            vram_framebuffer_start = start_hex;
            vram_framebuffer_end   = end_hex;
        }
    }

    if (ferror(maps))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: read error on /proc/self/maps because %s.",
                        strerror(errno));

        if (fclose(maps) != 0)
        {
            POUND_LOG_WARN(
                &thread_logger, "fclose(/proc/self/maps) also failed because %s.", strerror(errno));
        }

        return false;
    }

    if (fclose(maps) != 0)
    {
        POUND_LOG_WARN(
            &thread_logger, "fclose(/proc/self/maps) failed because %s.", strerror(errno));
    }

    if (0 == matched_regions)
    {
        POUND_LOG_WARN(&thread_logger, "No host program regions found in /proc/self/maps.");
        return false;
    }

#endif // POUND_PLATFORM_WINDOWS

    context->gva_low                    = low;
    context->gva_high                   = high;
    context->gva_text_start             = text_start;
    context->gva_text_end               = text_end;
    context->gva_data_start             = data_start;
    context->gva_data_end               = data_end;
    context->gva_vram_framebuffer_start = vram_framebuffer_start;
    context->gva_vram_framebuffer_end   = vram_framebuffer_end;
    return true;
}

void
debug_memory_gui_box_init(debug_memory_gui_box_t *box,
                          const ImVec2_c          position,
                          const float             width,
                          const float             height,
                          const ImU32             fill_color,
                          const char             *label,
                          const char             *caption)
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

ImU32
debug_memory_gui_box_lighten(const ImU32 color, const float amount)
{
    ImVec4_c rgba         = igColorConvertU32ToFloat4(color);
    rgba.x                = rgba.x + (1.0f - rgba.x) * amount;
    rgba.y                = rgba.y + (1.0f - rgba.y) * amount;
    rgba.z                = rgba.z + (1.0f - rgba.z) * amount;
    const ImU32 new_color = igColorConvertFloat4ToU32(rgba);
    return new_color;
}

/*** end of file ***/
