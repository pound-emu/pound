#ifndef POUND_MIMALLOC_TRACKER_H
#define POUND_MIMALLOC_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint64_t current_frame;
    uint64_t gva_high;
    uint64_t gva_low;
    uint64_t gva_text_start;
    uint64_t gva_text_end;
    uint64_t gva_data_start;
    uint64_t gva_data_end;
    bool     first_time_run;
    char     pad[7];
} debug_memory_tracker_t;

void gui_render_debug_memory_tracker(debug_memory_tracker_t *context);

#endif // POUND_MIMALLOC_TRACKER_H
