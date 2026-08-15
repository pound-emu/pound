#include "mimalloc_tracker.h"
#include <mimalloc-stats.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

void
gui_render_mimalloc_tracker(void)
{
    mi_stats_t_decl(stats);

    if (true == igBegin("Mimalloc Arena", NULL, 0))
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