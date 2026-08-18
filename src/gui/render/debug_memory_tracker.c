#include "debug_memory_tracker.h"
#include "log.h"
#include "mimalloc-stats.h"
#include "platform.h"
#include <errno.h>
#include <stdlib.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#if POUND_PLATFORM_POSIX

#define __USE_XOPEN_EXTENDED
#include <unistd.h>

#endif // POUND_PLATFORM_POSIX

#define TRACKER_PATH_CAPACITY 4096
#define TRACKER_LINE_CAPACITY 1024

static bool get_host_address_space_range(uint64_t *out_low, uint64_t *out_high);

void
gui_render_debug_memory_tracker(void)
{
    static uint64_t gva_low  = 0;
    static uint64_t gva_high = 0;
    static uint64_t frame    = 0;

    if (0 == (++frame & 127))
    {
        get_host_address_space_range(&gva_low, &gva_high);
    }

    char title[160];
    if (0 != gva_high)
    {
        snprintf(title,
                 sizeof(title),
                 "Guest Address Space - 0x%llx - 0x%llx (mapped view)",
                 (unsigned long long)gva_low,
                 (unsigned long long)gva_high);
    }
    else
    {
        snprintf(title, sizeof(title), "Guest Address Space - unknown");
    }

    mi_stats_t_decl(stats);

    if (true == igBegin(title, NULL, 0))
    {
        if (mi_stats_get(&stats))
        {
            igText(
                "reserved: %.1f MiB   committed: %.1f MiB   in use: %.1f KiB   malloc req: %.1f "
                "MiB",
                (double)stats.reserved.current / (1024.0 * 1024.0),
                (double)stats.committed.current / (1024.0 * 1024.0),
                (double)stats.malloc_requested.current / 1024.0,
                (double)stats.malloc_requested.total / (1024.0 * 1024.0));
        }
    }

    igEnd();
}

static bool
get_host_address_space_range(uint64_t *POUND_RESTRICT out_low, uint64_t *POUND_RESTRICT out_high)
{
    if (NULL == out_low)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out_low is NULL.");
        return false;
    }

    if (NULL == out_high)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out_high is NULL.");
        return false;
    }

    uint64_t low  = UINT64_MAX;
    uint64_t high = 0;

    // TODO: Support Windows Builds.
#if POUND_PLATFORM_POSIX

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
        const char *field = line_cursor;

        while ('\0' != *line_cursor && ' ' != *line_cursor)
        {
            ++line_cursor;
        }

        if (line_cursor == field)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Skipping /proc/self/maps line %llu: empty permissions field.",
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

#endif // POUND_PLATFORM_POSIX

    *out_low  = low;
    *out_high = high;
    return true;
}

/*** end of file ***/
