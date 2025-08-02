#ifndef POUND_GUI_H
#define POUND_GUI_H

#include <SDL3/SDL.h>
#include <vector>
#include "memory/arena.h"

namespace gui
{

#define WINDOW_MINIMUM_SIZE_WIDTH 640
#define WINDOW_MINIMUM_SIZE_HEIGHT 480

#define GUI_SUCCESS 0
#define WINDOW_SHOULD_CLOSE 1
#define ERROR_OPENGL 2

/*
 *  NAME
 *      window_t - Structure representing a window with OpenGL context.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      typedef struct {
 *          SDL_Window* data;               // Window created by the SDL library.
 *          SDL_GLContext gl_context;       // OpenGL context associated with the window.
 *      } window_t;
 *
 *  DESCRIPTION
 *      The window_t structure is used to represent a window along with its associated OpenGL context.
 *
 *  EXAMPLE
 *
 *  #include "gui/gui.h"
 *  #include <stdio.h>
 *
 *  int main() {
 *      gui::window_t window;
 *
 *      // Initialize the widow.
 *      if (!gui::window_init(&window, "Pound Emulator", 800, 600)) {
 *          fprintf(stderr, "Failed to initialize the window. \n");
 *          return -1;
 *      }
 *
 *      // Clean up when done.
 *      gui::window_destroy(&window);
 *  }
 */
typedef struct
{
    SDL_Window* data;
    SDL_GLContext gl_context;
} window_t;

/*
 *  NAME
 *      window_init - Initializes a window with specified properties.
 *
 *  SYNOPSIS
 *      #include "gui/gui"
 *
 *      bool gui::window_init(window_t* window, const char*  title, int64_t width, int64_t height);
 *
 *  DESCRIPTION
 *      The function initializes a window with the given parameters.
 *
 *  RETURN VALUE
 *      The function returns true if the window successfully initialized with all properties set, false otherwise.
 *
 *  NOTES
 *      - Ensure that the window is not null before calling this function.
 *      - The window will be created with OpenGL support.
 */
bool window_init(window_t* window, const char* title, int64_t width, int64_t height);

/*
 *  NAME
 *      window_destroy - Destroys a previously initialized window and its associated OpenGL context.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      void gui::window_destroy(gui::window_t* window);
 *
 *  DESCRIPTION
 *      The function cleans up resources associated with a previous initialized window.
 *
 *  NOTES
 *      - Ensure that the window parameter is valid and points to a previous initialized window structure.
 *      - It is essential to call this function for every window created with gui::window_init() to prevent resource leaks.
 */
void window_destroy(window_t* window);

/*
 *  NAME
 *      gui_t - Structure representing the main GUI system with window and panel management.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      typedef struct {
 *          window_t window;                  // Main window of the GUI with OpenGL context
 *          const char** custom_panels;       // Array of pointers to custom panel names
 *          bool* custom_panels_visibility;   // Array tracking visibility state of each panel
 *          size_t custom_panels_capacity;    // Maximum number of panels that can be managed
 *      } gui_t;
 *
 *  DESCRIPTION
 *      The gui_t structure represents the core GUI system, managing the main window and any
 *      additional user-defined panels. It provides a container for all resources needed to
 *      maintain and render the graphical interface.
 *
 *      This structure should be initialized before use and destroyed when no longer needed to
 *      properly manage memory and system resources.
 *
 *  EXAMPLE
 *
 *  #include "gui/gui.h"
 *  #include <stdio.h>
 *
 *  int main() {
 *      gui::gui_t gui;
 *
 *      // Initialize the GUI system
 *      if (!gui::init(&gui, &main_window)) {
 *          fprintf(stderr, "Failed to initialize the GUI. \n");
 *          return -1;
 *      }
 *
 *      // Use the GUI for rendering...
 *      ...
 *
 *      // Clean up when done.
 *      gui::destroy(&gui);
 *  }
 */
typedef struct
{
    window_t window;
    const char** custom_panels;
    bool* custom_panels_visibility;
    size_t custom_panels_capacity;
} gui_t;

/*
 *  NAME
 *      init_imgui - Initializes ImGui and its integration with SDL3 and OpenGL.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      bool gui::init_imgui(gui::window_t* main_window);
 *
 *  DESCRIPTION
 *      This function initializes the ImGui library along with its integration with
 *      SDL3 for input handling and OpenGL for rendering. It sets up all necessary
 *      configurations and context required for ImGui to work properly.
 *
 *  RETURN VALUE
 *      Returns true if ImGui initialization was successful, false otherwise.
 *
 *  NOTES
 *      - The function assumes that the main window has already been created and initialized
 *      - This should be called before any other ImGui functions are used
 */
bool init_imgui(gui::window_t* main_window);

/*
 *  NAME
 *      render_memu_bar - Renders a main menu bar with standard menus and custom panels.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      int8_t gui::render_memu_bar(const char** panels, const size_t panels_count,
 *                                 bool* panels_visibility, bool* imgui_demo_visible);
 *
 *  DESCRIPTION
 *      This function renders a main menu bar using ImGui that includes standard menus like File and View.
 *
 *      The File menu contains an Exit option that triggers window closure.
 *      The View menu displays all custom panels registered with their visibility toggles,
 *      plus an optional ImGui demo window toggle.
 *
 *  PARAMETERS
 *      panels - Array of panel names to be displayed in the View menu
 *      panels_count - Number of panels in the array
 *      panels_visibility - Boolean array indicating current visibility state for each panel
 *      imgui_demo_visible - Pointer to boolean controlling visibility of ImGui demo window
 *
 *  RETURN VALUE
 *      Returns one of these codes:
 *          GUI_SUCCESS - Normal operation
 *          WINDOW_SHOULD_CLOSE - Exit option was selected
 *
 *  NOTES
 *      - Panel visibility state is toggled when corresponding menu items are clicked
 */
int8_t render_memu_bar(const char** panels, size_t panels_count, bool* panels_visibility, bool* imgui_demo_visible);

/*
 *  NAME
 *      destroy - Destroys a GUI system and cleans up resources.
 *
 *  SYNOPSIS
 *      #include "gui/gui.h"
 *
 *      void gui::destroy();
 *
 *  DESCRIPTION
 *      The function cleans up and releases all resources associated with the GUI system.
 */
void destroy();

}  // namespace gui

#endif  //POUND_GUI_H
