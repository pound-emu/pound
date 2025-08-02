#ifndef POUND_PANELS_H
#define POUND_PANELS_H

#include <chrono>
#include <deque>

namespace gui::panel
{
#define PANEL_NAME_CPU "Cpu"
#define PANEL_NAME_PERFORMANCE "Performance"
#define FRAME_TIME_HISTORY_SIZE 128

#define PANEL_SUCCESS 0
#define ERROR_PANEL_IS_CLOSED 1

/*
 *  NAME
 *      performance_panel_t - Structure for tracking performance metrics history.
 *
 *  SYNOPSIS
 *      #include "gui/panels.h"
 *
 *      typedef struct {
 *          std::deque<float_t> fps_history;        // Historical FPS values over time
 *          std::deque<float_t> frame_time_history; // Historical frame time values over time
 *      } performance_panel_t;
 *
 *  DESCRIPTION
 *      The performance_panel_t structure maintains a history of performance metrics for visualization and analysis.
 */
typedef struct
{
    std::deque<float_t> fps_history;
    std::deque<float_t> frame_time_history;
} performance_panel_t;

/*
 *  NAME
 *      performance_data_t - Structure for storing current performance metrics.
 *
 *  SYNOPSIS
 *      #include "gui/panels.h"
 *
 *      typedef struct {
 *          float_t fps;               // Current frames per second
 *          float_t frame_time;        // Time taken to render a single frame in seconds
 *          float_t cpu_usage;         // CPU usage percentage
 *          float_t memory_usage;      // Memory usage percentage
 *          int32_t frame_count;       // Total number of frames rendered since startup
 *      } performance_data_t;
 *
 *  DESCRIPTION
 *      The performance_data_t structure contains current runtime metrics per frame for monitoring
 *      application performance.
 */
typedef struct
{
    float_t fps;
    float_t frame_time;
    float_t cpu_usage;
    float_t memory_usage;
    int32_t frame_count;

} performance_data_t;

/*
 *  NAME
 *      render_performance_panel - Renders a performance monitoring panel with FPS and system metrics.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      int8_t gui::panel::render_performance_panel(gui::panel::performance_panel_t* panel,
 *                                                 performance_data_t* data,
 *                                                 std::chrono::steady_clock::time_point* last_render);
 *
 *  DESCRIPTION
 *      This function renders a performance monitoring panel that tracks and displays:
 *          - Real-time FPS (frames per second) calculation
 *          - Frame time statistics with historical graph visualization
 *          - CPU and memory usage metrics (placeholder values)
 *          - Emulation statistics (placeholder values)
 *
 *
 *  RETURN VALUE
 *      Returns one of these codes:
 *          PANEL_SUCCESS - Normal operation
 *          ERROR_PANEL_IS_CLOSED - Panel was closed by user
 *
 */
int8_t render_performance_panel(performance_panel_t* panel, performance_data_t* data,
                                std::chrono::steady_clock::time_point* last_render);

/*
 *  NAME
 *      render_cpu_panel - Renders a CPU testing panel with test execution capability.
 *
 *  SYNOPSIS
 *      #include "gui/panels.cpp"
 *
 *      int8_t gui::panel::render_cpu_panel(bool* show_cpu_result_popup);
 *
 *  DESCRIPTION
 *      This function renders a CPU testing panel.
 *
 *  RETURN VALUE
 *      Returns one of these codes:
 *          PANEL_SUCCESS - Normal operation
 *          ERROR_PANEL_IS_CLOSED - Panel was closed by user
 */
int8_t render_cpu_panel(bool* show_cpu_result_popup);
}  // namespace gui::panel
#endif  //POUND_PANELS_H
