// Copyright 2025 Pound Emulator Project. All rights reserved.

#include <chrono>
#include <memory>
#include <thread>

#define LOG_MODULE "main"
#include "common/logging.h"
#include "common/passert.h"
#include "frontend/gui.h"
#include "host/memory/arena.h"

#include <SDL3/SDL_opengl.h>
#include "frontend/color.h"
#include "frontend/panels.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

int main()
{
#if 1
    gui::window_t window = {.data = nullptr, .gl_context = nullptr};
    (void)gui::window_init(&window, "Pound Emulator", 640, 480);

    if (bool return_code = gui::init_imgui(&window); false == return_code)
    {
        LOG_ERROR( "Failed to initialize GUI");
        return EXIT_FAILURE;
    }

    const size_t panels_capacity = 2;
    const char* panel_names[panels_capacity] = {PANEL_NAME_CPU, PANEL_NAME_PERFORMANCE};
    bool panels_visibility[panels_capacity] = {};
    bool imgui_demo_visible = false;

    gui::gui_t gui = {
        .window = window,
        .custom_panels = panel_names,
        .custom_panels_visibility = panels_visibility,
        .custom_panels_capacity = panels_capacity,
    };

    gui::panel::performance_panel_t performance_panel = {};
    gui::panel::performance_data_t performance_data = {.frame_count = 1};
    std::chrono::steady_clock::time_point performance_panel_last_render = std::chrono::steady_clock::now();

    // Main loop
    bool is_running = true;
    bool show_cpu_result_popup = false;
    while (true == is_running)
    {
        SDL_Event event = {};
        while (::SDL_PollEvent(&event))
        {
            (void)::ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                is_running = false;
            }
        }

        ::ImGui_ImplOpenGL3_NewFrame();
        ::ImGui_ImplSDL3_NewFrame();
        ::ImGui::NewFrame();
        if (int8_t return_code = gui::render_memu_bar(gui.custom_panels, gui.custom_panels_capacity,
                                                      gui.custom_panels_visibility, &imgui_demo_visible);
            WINDOW_SHOULD_CLOSE == return_code)
        {
            is_running = false;
        }

        for (size_t i = 0; i < panels_capacity; ++i)
        {
            if (false == gui.custom_panels_visibility[i])
            {
                continue;
            }

            if (0 == ::strcmp(gui.custom_panels[i], PANEL_NAME_PERFORMANCE))
            {
                int8_t return_code = gui::panel::render_performance_panel(&performance_panel, &performance_data,
                                                                          &performance_panel_last_render);
                if (ERROR_PANEL_IS_CLOSED == return_code)
                {
                    gui.custom_panels_visibility[i] = false;
                }
            }
            if (0 == ::strcmp(gui.custom_panels[i], PANEL_NAME_CPU))
            {
                int8_t return_code = gui::panel::render_cpu_panel(&show_cpu_result_popup);
                if (ERROR_PANEL_IS_CLOSED == return_code)
                {
                    gui.custom_panels_visibility[i] = false;
                }
            }
        }

        // End Frame.
        ImGui::Render();
        const ImGuiIO& io = ImGui::GetIO();
        ::glViewport(0, 0, static_cast<GLint>(io.DisplaySize.x), static_cast<GLint>(io.DisplaySize.y));
        ::glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT);

        ::ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ::SDL_GL_SwapWindow(gui.window.data);


        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    gui::destroy();
#endif
}
