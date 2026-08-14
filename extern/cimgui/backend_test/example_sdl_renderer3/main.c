#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include <cimgui.h>
#include <cimgui_impl.h>

#define igGetIO igGetIO_Nil

int
main()
{
    // Setup SDL library
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        fprintf(stderr, "Failed to init video! %s", SDL_GetError());
        return 1;
    };

    // Setup window and renderer
    float           main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags
        = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window   *window   = NULL;
    SDL_Renderer *renderer = NULL;
    if (!SDL_CreateWindowAndRenderer("Dear ImGui SDL3 Renderer example",
                                     (int)(1280 * main_scale),
                                     (int)(720 * main_scale),
                                     window_flags,
                                     &window,
                                     &renderer))
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    igCreateContext(NULL);
    ImGuiIO *io = igGetIO();
    (void)io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    igStyleColorsDark(NULL);
    // igStyleColorsLight(NULL);

    // Setup scaling
    ImGuiStyle *style = igGetStyle();
    ImGuiStyle_ScaleAllSizes(style, main_scale);
    style->FontScaleDpi
        = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
                      // unnecessary. We leave both here for documentation purpose)
    io->ConfigDpiScaleFonts
        = true; // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor
                // DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io->ConfigDpiScaleViewports
        = true; // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Our state
    bool   show_demo_window    = true;
    bool   show_another_window = false;
    ImVec4 clear_color;
    clear_color.x = 0.45f;
    clear_color.y = 0.55f;
    clear_color.z = 0.60f;
    clear_color.w = 1.00f;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui
        // wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
        // application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
        // application, or clear/overwrite your copy of the keyboard data. Generally you may always
        // pass all inputs to dear imgui, and hide them from your application based on those two
        // flags. [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your
        // SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate()
        // function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Setup Dear ImGui frame
        SDL_SetRenderDrawColorFloat(
            renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        // 1. Show the big demo window (Most of the sample code is in igShowDemoWindow()! You can
        // browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
        {
            igShowDemoWindow(&show_demo_window);
        }

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a
        // named window.
        {
            static float f       = 0.0f;
            static int   counter = 0;

            igBegin("Hello, world!",
                    NULL,
                    0); // Create a window called "Hello, world!" and append into it.

            igText("This is some useful text."); // Display some text (you can use a format strings
                                                 // too)
            igCheckbox("Demo Window",
                       &show_demo_window); // Edit bools storing our window open/close state
            igCheckbox("Another Window", &show_another_window);

            igSliderFloat("float",
                          &f,
                          0.0f,
                          1.0f,
                          "%.3f",
                          0); // Edit 1 float using a slider from 0.0f to 1.0f
            igColorEdit4(
                "clear color", (float *)&clear_color, 0); // Edit 3 floats representing a color

            ImVec2 buttonSize;
            buttonSize.x = 0;
            buttonSize.y = 0;
            if (igButton("Button", buttonSize)) // Buttons return true when clicked (most widgets
                                                // return true when edited/activated)
            {
                counter++;
            }
            igSameLine(0.0f, -1.0f);
            igText("counter = %d", counter);

            igText("Application average %.3f ms/frame (%.1f FPS)",
                   1000.0f / io->Framerate,
                   io->Framerate);
            igEnd();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            igBegin("Another Window",
                    &show_another_window,
                    0); // Pass a pointer to our bool variable (the window will have a closing
                        // button that will clear the bool when clicked)
            igText("Hello from another window!");
            ImVec2 buttonSize;
            buttonSize.x = 0;
            buttonSize.y = 0;
            if (igButton("Close Me", buttonSize))
            {
                show_another_window = false;
            }
            igEnd();
        }

        // Rendering
        igRender();
        ImDrawData *draw_data = igGetDrawData();
        const bool  is_minimized
            = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        ImGui_ImplSDLRenderer3_RenderDrawData(draw_data, renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    igDestroyContext(NULL);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
