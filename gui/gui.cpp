#include "gui.h"
#include "Base/Assert.h"
#include "Base/Logging/Log.h"
#include "color.h"
#include "imgui_impl_opengl3_loader.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

static void apply_theme();

//=========================================================
// PUBLIC FUNCTIONS
//=========================================================

bool gui::window_init(window_t* window, const char* title, int64_t width, int64_t height)
{
    ASSERT(nullptr != window);
    ASSERT(nullptr != title);

    bool ret = ::SDL_Init(SDL_INIT_VIDEO);
    if (false == ret)
    {
        LOG_ERROR(Render, "Error creating SDL3 Context: {}", SDL_GetError());
        return false;
    }

    SDL_PropertiesID properties = ::SDL_CreateProperties();
    if (0 == properties)
    {
        LOG_ERROR(Render, "Error creating SDL3 Properties: {}", SDL_GetError());
        return false;
    }

    ret = ::SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    if (false == ret)
    {
        LOG_ERROR(Render, "Error setting window title {}: {}", title, SDL_GetError());
        return false;
    }

    (void)::SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    (void)::SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);

    ret = ::SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    if (false == ret)
    {
        LOG_ERROR(Render, "Error setting window {} width {}: ", title, width, SDL_GetError());
        return false;
    }

    ret = ::SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    if (false == ret)
    {
        LOG_ERROR(Render, "Error setting window {} height {}: ", title, height);
        return false;
    }

    (void)::SDL_SetNumberProperty(properties, "flags", SDL_WINDOW_OPENGL);
    (void)::SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    (void)::SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);

    window->data = ::SDL_CreateWindowWithProperties(properties);
    ::SDL_DestroyProperties(properties);
    if (nullptr == window->data)
    {
        LOG_ERROR(Render, "Failed to create window {}: {}", title, SDL_GetError());
        return false;
    }

    ret = ::SDL_SetWindowMinimumSize(window->data, WINDOW_MINIMUM_SIZE_WIDTH, WINDOW_MINIMUM_SIZE_HEIGHT);
    if (false == ret)
    {
        LOG_ERROR(Render, "Failed to set window {} minimum width and height: {}", title, SDL_GetError());
        return false;
    }

    window->gl_context = ::SDL_GL_CreateContext(window->data);
    if (nullptr == window->gl_context)
    {
        LOG_ERROR(Render, "Failed to create OpenGL context: {}", SDL_GetError());
        return false;
    }

    ret = ::SDL_GL_MakeCurrent(window->data, window->gl_context);
    if (false == ret)
    {
        LOG_ERROR(Render, "Failed to make set OpenGL context to window {}: {}", title, SDL_GetError());
        return false;
    }

    ret = ::SDL_GL_SetSwapInterval(1);
    if (false == ret)
    {
        LOG_ERROR(Render, "Failed to set swap interval for window {}: {}", title, SDL_GetError());
        return false;
    }

    return true;
}

void gui::window_destroy(gui::window_t* window)
{
    bool ret = false;
    if (window->gl_context != nullptr)
    {
        ret = ::SDL_GL_DestroyContext(window->gl_context);
        if (false == ret)
        {
            LOG_ERROR(Render, "Failed to destroy OpenGL context");
        }
    }
    if (window->data != nullptr)
    {
        ::SDL_DestroyWindow(window->data);
    }
}

bool gui::init_imgui(gui::window_t* main_window)
{
    ASSERT(nullptr != main_window->data);
    ASSERT(nullptr != main_window->gl_context);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    (void)::ImGui::CreateContext();
    ImGuiIO& io = ::ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ::apply_theme();

    bool ret = ::ImGui_ImplSDL3_InitForOpenGL(main_window->data, main_window->gl_context);
    if (false == ret)
    {
        LOG_ERROR(Render, "Failed to init SDL3: {}", SDL_GetError());
        return false;
    }

#ifdef __APPLE__ && (__aarch64__)
    ret = ::ImGui_ImplOpenGL3_Init("#version 120");
#elif __APPLE__ && (__x86_64__)
    ret = ::ImGui_ImplOpenGL3_Init("#version 150");
#else
    ret = ::ImGui_ImplOpenGL3_Init("#version 330");
#endif
    if (false == ret)
    {
        LOG_ERROR(Render, "Failed to init OpenGL3: {}", SDL_GetError());
        return false;
    }

    return true;
}

int8_t gui::render_memu_bar(const char** panels, const size_t panels_count, bool* panels_visibility,
                            bool* imgui_demo_visible)
{
    int8_t return_code = GUI_SUCCESS;
    if (true == ::ImGui::BeginMainMenuBar())
    {
        if (true == ::ImGui::BeginMenu("File"))
        {
            ::ImGui::Separator();
            if (true == ::ImGui::MenuItem("Exit", "Alt+F4"))
            {
                return_code = WINDOW_SHOULD_CLOSE;
            }
            ::ImGui::EndMenu();
        }
        if (true == ::ImGui::BeginMenu("View"))
        {
            for (size_t i = 0; i < panels_count; ++i)
            {
                (void)::ImGui::MenuItem(panels[i], nullptr, &panels_visibility[i]);
            }

            ::ImGui::Separator();
            // The demo window will need to be rendered outside this nested if statement, or else it will close the next frame.
            (void)::ImGui::MenuItem("ImGui Demo", nullptr, imgui_demo_visible);
            ::ImGui::EndMenu();
        }

        ::ImGui::EndMainMenuBar();
    }

    if (true == *imgui_demo_visible)
    {
        ::ImGui::ShowDemoWindow(imgui_demo_visible);
    }

    return return_code;
}

void gui::destroy()
{
    ::ImGui_ImplOpenGL3_Shutdown();
    ::ImGui_ImplSDL3_Shutdown();
    ::ImGui::DestroyContext();
}

//=========================================================
// Private FUNCTIONS
//=========================================================

void apply_theme()
{
    ImGuiStyle& style = ::ImGui::GetStyle();

    // Modern theme with custom colors
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;

    // Apply custom color scheme.
    style.Colors[ImGuiCol_Text] = gui::color::text;
    style.Colors[ImGuiCol_TextDisabled] = gui::color::text_disable;
    style.Colors[ImGuiCol_WindowBg] = gui::color::with_alpha(gui::color::background, 0.95F);
    style.Colors[ImGuiCol_ChildBg] = gui::color::background_dark;
    style.Colors[ImGuiCol_PopupBg] = gui::color::with_alpha(gui::color::background, 0.94F);
    style.Colors[ImGuiCol_Border] = gui::color::border;
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);
    style.Colors[ImGuiCol_FrameBg] = gui::color::background_light;
    style.Colors[ImGuiCol_FrameBgHovered] = gui::color::lighten(gui::color::background_light, 0.1F);
    style.Colors[ImGuiCol_FrameBgActive] = gui::color::lighten(gui::color::background_light, 0.2F);
    style.Colors[ImGuiCol_TitleBg] = gui::color::background_dark;
    style.Colors[ImGuiCol_TitleBgActive] = gui::color::background;
    style.Colors[ImGuiCol_TitleBgCollapsed] = gui::color::with_alpha(gui::color::background_dark, 0.51F);
    style.Colors[ImGuiCol_MenuBarBg] = gui::color::background_dark;
    style.Colors[ImGuiCol_ScrollbarBg] = gui::color::with_alpha(gui::color::background_dark, 0.53F);
    style.Colors[ImGuiCol_ScrollbarGrab] = gui::color::background_light;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = gui::color::lighten(gui::color::background_light, 0.1F);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = gui::color::lighten(gui::color::background_light, 0.2F);
    style.Colors[ImGuiCol_CheckMark] = gui::color::primary;
    style.Colors[ImGuiCol_SliderGrab] = gui::color::primary;
    style.Colors[ImGuiCol_SliderGrabActive] = gui::color::primary_active;
    style.Colors[ImGuiCol_Button] = gui::color::with_alpha(gui::color::primary, 0.4F);
    style.Colors[ImGuiCol_ButtonHovered] = gui::color::primary_hover;
    style.Colors[ImGuiCol_ButtonActive] = gui::color::primary_active;
    style.Colors[ImGuiCol_Header] = gui::color::with_alpha(gui::color::primary, 0.4F);
    style.Colors[ImGuiCol_HeaderHovered] = gui::color::with_alpha(gui::color::primary, 0.8F);
    style.Colors[ImGuiCol_HeaderActive] = gui::color::primary;
    style.Colors[ImGuiCol_Separator] = gui::color::border;
    style.Colors[ImGuiCol_SeparatorHovered] = gui::color::with_alpha(gui::color::primary, 0.78F);
    style.Colors[ImGuiCol_SeparatorActive] = gui::color::primary;
    style.Colors[ImGuiCol_ResizeGrip] = gui::color::with_alpha(gui::color::primary, 0.25F);
    style.Colors[ImGuiCol_ResizeGripHovered] = gui::color::with_alpha(gui::color::primary, 0.67F);
    style.Colors[ImGuiCol_ResizeGripActive] = gui::color::with_alpha(gui::color::primary, 0.95F);
    style.Colors[ImGuiCol_Tab] = gui::color::background_light;
    style.Colors[ImGuiCol_TabHovered] = gui::color::with_alpha(gui::color::primary, 0.8F);
    style.Colors[ImGuiCol_TabActive] = gui::color::primary;
    style.Colors[ImGuiCol_TabUnfocused] = gui::color::background;
    style.Colors[ImGuiCol_TabUnfocusedActive] = gui::color::lighten(gui::color::background, 0.1F);
    style.Colors[ImGuiCol_PlotLines] = gui::color::primary;
    style.Colors[ImGuiCol_PlotLinesHovered] = gui::color::primary_hover;
    style.Colors[ImGuiCol_PlotHistogram] = gui::color::secondary;
    style.Colors[ImGuiCol_PlotHistogramHovered] = gui::color::secondary_hover;
    style.Colors[ImGuiCol_TextSelectedBg] = gui::color::with_alpha(gui::color::primary, 0.35F);
    style.Colors[ImGuiCol_DragDropTarget] = gui::color::with_alpha(gui::color::secondary, 0.9F);
    style.Colors[ImGuiCol_NavHighlight] = gui::color::primary;
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
