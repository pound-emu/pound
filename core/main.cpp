// Copyright 2025 Pound Emulator Project. All rights reserved.

#include <thread>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include "Base/Logging/Backend.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

//#include "ARM/cpu.h"
#include "Base/Config.h"
//#include "JIT/jit.h"
#include "Base/Logging/Log.h"


SDL_Window* Window{};
SDL_GLContext gl_context{};
SDL_Event windowEvent;

void initSDL3() {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR(Render, "Error while creating SDL3 Context!");
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Pound Emulator");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, Config::windowWidth());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, Config::windowHeight());
    // For a new Vulkan support, don't forget to change 'SDL_WINDOW_OPENGL' by 'SDL_WINDOW_VULKAN'.
    SDL_SetNumberProperty(props, "flags", SDL_WINDOW_OPENGL);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    Window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    SDL_SetWindowMinimumSize(Window, 640, 480);

    gl_context = SDL_GL_CreateContext(Window);
    if (!gl_context) {
        LOG_ERROR(Render, "Failed to create OpenGL context: {}", SDL_GetError());
    }

    SDL_GL_MakeCurrent(Window, gl_context);
    SDL_GL_SetSwapInterval(1);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(Window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

}

void deinitializeSDL3() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(Window);
    SDL_Quit();
}

/* using REM instead
void cpuTest() {
    CPU cpu;
    cpu.pc = 0;

    // Simple ARMv8 program in memory (MOVZ X0, #5; ADD X0, X0, #3; RET)
    // These are placeholders; real encoding will be parsed later
    cpu.write_byte(0, 0x05); // MOVZ placeholder
    cpu.write_byte(4, 0x03); // ADD placeholder
    cpu.write_byte(8, 0xFF); // RET placeholder

    LOG_INFO(ARM, "{}", cpu.read_byte(0));

    JIT jit;
    jit.translate_and_run(cpu);

    cpu.print_debug_information();

    LOG_INFO(ARM, "X0 = {}", cpu.x(0));
}
*/

int main() {
    Base::Log::Initialize();
    Base::Log::Start();

    const auto config_dir = Base::FS::GetUserPath(Base::FS::PathType::BinaryDir);
    Config::Load(config_dir / "config.toml");

    initSDL3();

    bool rendering = true;
    bool show_popup = false; // Flag to control popup visibility

    while (rendering) {
        // Process events.
        while (SDL_PollEvent(&windowEvent)) {
            ImGui_ImplSDL3_ProcessEvent(&windowEvent);

            switch (windowEvent.type) {
            case SDL_EVENT_QUIT:
                rendering = false;
                break;
            default:
                break;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::SetNextWindowSizeConstraints(ImVec2(50.0f, 50.0f), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::NewFrame();

        ImGui::Begin("Pound Emulator");

        if (ImGui::Button("Run CPU Test")) {
            //cpuTest();  // outdated cpu implementation 
            show_popup = true; // Trigger the popup after running the test
        }

        ImGui::End();

        // Popup logic
        if (show_popup) {
            ImGui::OpenPopup("CPU Test Info");
        }

        // Define the modal popup
        if (ImGui::BeginPopupModal("CPU Test Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("The CPU test has ran in the terminal.\nKeep in mind that Pound is still in pre-alpha state.");
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                show_popup = false; // Close the popup
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(Window);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    deinitializeSDL3();
    return 0;
}
