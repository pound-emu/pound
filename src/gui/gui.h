#ifndef POUND_GUI_H
#define POUND_GUI_H

#include "attributes.h"
#include "platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_PATH         4096
#define FILE_BUFFER_SIZE 65536

#if POUND_PLATFORM_WINDOWS

#define GUI_PLUGIN_NAME "libPoundGui.dll"

#else

#define GUI_PLUGIN_NAME "libPoundGui.so"

#endif // POUND_PLATFORM_WINDOWS

typedef struct
{
    void *(*create)(const void *saved_state, size_t saved_size);
    void (*destroy)(void *gui);
    void (*render_frame)(void *gui);
    size_t (*save)(void *gui, void *out, size_t capacity);
} gui_exports_t;

typedef struct
{
    gui_exports_t exports;
    void         *module;
    void         *gui_handle;
    char          loaded_path[MAX_PATH];
    bool          loaded;
    char          pad[7];
} gui_plugin_t;

POUND_EXPORT bool     gui_plugin_load_module(gui_plugin_t *POUND_RESTRICT plugin,
                                             const char *POUND_RESTRICT   source_path);
POUND_EXPORT void     gui_plugin_destroy(gui_plugin_t *plugin);
POUND_EXPORT bool     gui_exports_get(gui_exports_t *out);
POUND_EXPORT uint64_t file_modified_time(const char *path);
POUND_EXPORT bool     copy_file(const char *POUND_RESTRICT source,
                                const char *POUND_RESTRICT destination);

#endif // POUND_GUI_H

/*** end of file ***/
