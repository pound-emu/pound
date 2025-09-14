#include "panels.h"
#include <imgui.h>
#include <math.h>
#include "kvm/kvm.h"
#include <assert.h>"

int8_t gui::panel::render_performance_panel(gui::panel::performance_panel_t* panel, performance_data_t* data,
                                            std::chrono::steady_clock::time_point* last_render)
{
    assert(nullptr != panel);
    assert(nullptr != data);
    assert(nullptr != last_render);

    bool is_visible = true;
    (void)::ImGui::Begin(PANEL_NAME_PERFORMANCE, &is_visible);
    if (false == is_visible)
    {
        ::ImGui::End();
        return ERROR_PANEL_IS_CLOSED;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - *last_render);
    ++data->frame_count;
    if (duration.count() >= 100)
    {
        // Every 100ms
        data->fps = data->frame_count * 1000.0f / duration.count();
        data->frame_time = duration.count() / (float)data->frame_count;

        panel->fps_history.push_back(data->fps);
        panel->frame_time_history.push_back(data->frame_time);

        // Keep history size limited
        while (panel->fps_history.size() > FRAME_TIME_HISTORY_SIZE)
        {
            panel->fps_history.pop_front();
        }
        while (panel->frame_time_history.size() > FRAME_TIME_HISTORY_SIZE)

        {
            panel->frame_time_history.pop_front();
        }

        data->frame_count = 0;
        *last_render = now;

        // TODO(GloriousTaco:gui): Get actual CPU and memory usage
        data->cpu_usage = 0.0f;
        data->memory_usage = 0.0f;
    }
    ::ImGui::Text("FPS: %.1f", data->fps);
    ::ImGui::Text("Frame Time: %.2f ms", data->frame_time);
    ::ImGui::Separator();

    // Frame Time Graph
    if (false == panel->frame_time_history.empty())
    {
        float frame_time_array[FRAME_TIME_HISTORY_SIZE] = {};
        (void)std::copy(panel->frame_time_history.begin(), panel->frame_time_history.end(), frame_time_array);

        ::ImGui::Text("Frame Time History (ms):");
        ::ImGui::PlotLines("##FrameTime", frame_time_array, (int)panel->frame_time_history.size(), 0, nullptr, 0.0f,
                           33.33f, ImVec2(0, 80));
    }

    ::ImGui::Separator();

    // System info (placeholder)
    ::ImGui::Text("CPU Usage: %.1f%%", data->cpu_usage);
    ::ImGui::Text("Memory Usage: %.1f MB", data->memory_usage);

    // Emulation stats
    ::ImGui::Separator();
    ::ImGui::Text("Emulation Statistics:");
    ::ImGui::Text("Instructions/sec: N/A");
    ::ImGui::Text("JIT Cache Usage: N/A");

    ::ImGui::End();
    return PANEL_SUCCESS;
}

int8_t gui::panel::render_cpu_panel(bool* show_cpu_result_popup)
{
    assert(nullptr != show_cpu_result_popup);

    bool is_visible = true;
    (void)::ImGui::Begin(PANEL_NAME_CPU, &is_visible, ImGuiWindowFlags_NoCollapse);
    if (false == is_visible)
    {
        ::ImGui::End();
        return ERROR_PANEL_IS_CLOSED;
    }

    if (::ImGui::Button("Run CPU Test", ImVec2(120, 0)))
    {
        pound::kvm::cpuTest();
        *show_cpu_result_popup = true;
    }
    if (true == *show_cpu_result_popup)
    {
        ::ImGui::OpenPopup("CPU Test Result");
    }

    if (::ImGui::BeginPopupModal("CPU Test Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ::ImGui::Text("The CPU test has been executed successfully!");
        ::ImGui::Text("Check the console for detailed output.");
        ::ImGui::Separator();
        ::ImGui::Text("Note: Pound is still in pre-alpha state.");

        ::ImGui::Spacing();

        if (::ImGui::Button("OK", ImVec2(120, 0)))
        {
            *show_cpu_result_popup = false;
            ::ImGui::CloseCurrentPopup();
        }

        ::ImGui::EndPopup();
    }

    ::ImGui::End();
    return PANEL_SUCCESS;
}
