#include "gui.h"
#include "log.h"

#include "mimalloc-override.h"
#include <stdio.h>
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#if POUND_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else

#include <dlfcn.h>
#include <sys/stat.h>

#endif // POUND_PLATFORM_WINDOWS

typedef struct
{
    int  selected_tab;
    bool show_demo;
    bool show_hot_reload_panel;
    char pad[2];
} gui_state_t;

typedef bool (*get_exports_function_t)(gui_exports_t *);

static void    *gui_create(const void *saved_data, size_t saved_size);
static void     gui_destroy(void *gui_state);
static void     gui_render_frame(void *gui_state);
static size_t   gui_save(void *gui_state, void *out, size_t capacity);
static void    *shared_library_load(const char *path);
static void     shared_library_unload(void *module);
static bool     shared_library_get_symbol(void *module, const char *name, void **out);
static bool     shared_library_create_loaded_path(const char *source,
                                                  char       *destination,
                                                  size_t      capacity,
                                                  uint32_t    id);
static uint32_t hot_counter = 0;

bool
gui_plugin_load_module(gui_plugin_t *POUND_RESTRICT plugin, const char *POUND_RESTRICT source_path)
{
    if (NULL == plugin)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: plugin is NULL.");
        return false;
    }

    if (NULL == source_path)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: source_path is NULL.");
        return false;
    }

    if ('\0' == source_path[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: source_path is empty.");
        return false;
    }

    if (strlen(source_path) >= MAX_PATH)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: source_path exceeds MAX_PATH (%d).", MAX_PATH);
        return false;
    }

    memset(plugin, 0, sizeof(*plugin));
    ++hot_counter;
    char loaded_path[MAX_PATH];

    if (false
        == shared_library_create_loaded_path(
            source_path, loaded_path, sizeof(loaded_path), hot_counter))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: failed to create loaded path from %s", source_path);
        return false;
    }

    if ('\0' == loaded_path[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: generated loaded_path is empty.");
        return false;
    }

    if (false == copy_file(source_path, loaded_path))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to copy '%s' to '%s'.",
                        source_path,
                        loaded_path);
        return false;
    }

    void *POUND_RESTRICT module = shared_library_load(loaded_path);

    if (NULL == module)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: failed to load shared library '%s'.", loaded_path);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger,
                           "Aborting function: also failed to remove stale file '%s'.",
                           loaded_path);
        }

        return false;
    }

    void *symbol = NULL;

    if (false == shared_library_get_symbol(module, "gui_exports_get", &symbol))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: symbol 'gui_exports_get' not found in '%s'.",
                        loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(
                &thread_logger, "Failed to remove '%s' after symbol failure.", loaded_path);
        }
        return false;
    }

    if (NULL == symbol)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: 'gui_exports_get' resolved to NULL in '%s'.",
                        loaded_path);

        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        return false;
    }

    get_exports_function_t get_exports = NULL;
    memcpy(&get_exports, &symbol, sizeof(get_exports));

    if (NULL == get_exports)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: get_exports function pointer is NULL in '%s'.",
                        loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        return false;
    }

    gui_exports_t exports = { 0 };

    if (false == get_exports(&exports))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: gui_exports_get() returned false for '%s'.",
                        loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        return false;
    }

    if (NULL == exports.create)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: exports.create is NULL in '%s'.", loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }
        return false;
    }

    if (NULL == exports.destroy)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: exports.destroy is NULL in '%s'.", loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        return false;
    }

    if (NULL == exports.render_frame)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: exports.render_frame is NULL in '%s'.",
                        loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        return false;
    }

    if (NULL == exports.save)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: exports.save is NULL in '%s'.", loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        return false;
    }

    const int written
        = snprintf(plugin->loaded_path, sizeof(plugin->loaded_path), "%s", loaded_path);

    if (written < 0 || (size_t)written >= sizeof(plugin->loaded_path))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: loaded_path truncated, unloading module '%s'.",
                        loaded_path);
        shared_library_unload(module);

        if (remove(loaded_path) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "Failed to remove '%s'.", loaded_path);
        }

        memset(plugin, 0, sizeof(*plugin));
        return false;
    }

    plugin->module     = module;
    plugin->exports    = exports;
    plugin->gui_handle = NULL;
    plugin->loaded     = true;

    return true;
}

void
gui_plugin_destroy(gui_plugin_t *plugin)
{
    if (NULL == plugin)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: plugin is NULL.");
        return;
    }

    if (plugin->gui_handle != NULL && plugin->exports.destroy != NULL)
    {
        plugin->exports.destroy(plugin->gui_handle);
    }
    if (plugin->module != NULL)
    {
        shared_library_unload(plugin->module);
    }

    if (plugin->loaded_path[0])
    {
        remove(plugin->loaded_path);
    }

    memset(plugin, 0, sizeof(*plugin));
}

bool
gui_exports_get(gui_exports_t *out)
{
    if (NULL == out)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out is NULL.");
        return false;
    }

    out->create       = gui_create;
    out->destroy      = gui_destroy;
    out->render_frame = gui_render_frame;
    out->save         = gui_save;
    return true;
}

uint64_t
file_modified_time(const char *path)
{
    if (NULL == path)
    {
        POUND_LOG_WARN(&thread_logger, "Path is NULL, returning 0.");
        return 0;
    }

    if ('\0' == path[0])
    {
        POUND_LOG_WARN(&thread_logger, "Path is empty string, returning 0.");
        return 0;
    }

    if (strlen(path) >= MAX_PATH)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Path length exceeds MAX_PATH (%d), returning 0.", MAX_PATH);
        return 0;
    }

#if defined(_WIN32)

    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
    {
        return 0;
    }

    ULARGE_INTEGER ull;
    ull.LowPart  = data.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = data.ftLastWriteTime.dwHighDateTime;

    if (0 == ull.QuadPart)
    {
        POUND_LOG_DEBUG(
            &thread_logger, "file_modified_time: '%s' has zero modification time.", path);
    }

    return (uint64_t)ull.QuadPart;

#else

    struct stat st;

    if (stat(path, &st) != 0)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: stat() failed for '%s'.", path);
        return 0;
    }

    if (st.st_mtime < 0)
    {
        POUND_LOG_WARN(&thread_logger, "Aborting function: '%s' has negative mtime.", path);
        return 0;
    }

    return (uint64_t)st.st_mtime;

#endif
}

bool
copy_file(const char *source, const char *destination)
{
    if (NULL == source)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: source is NULL.");
        return false;
    }

    if (NULL == destination)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: destination is NULL.");
        return false;
    }

    if ('\0' == source[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: source is empty string.");
        return false;
    }

    if ('\0' == destination[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: destination is empty string.");
        return false;
    }

    if (strlen(source) >= MAX_PATH)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: source path exceeds MAX_PATH (%d).", MAX_PATH);
        return false;
    }

    if (strlen(destination) >= MAX_PATH)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: destination path exceeds MAX_PATH (%d).", MAX_PATH);
        return false;
    }

    FILE *in = fopen(source, "rb");

    if (NULL == in)
    {
        POUND_LOG_WARN(
            &thread_logger, "Aborting function: failed to open source '%s' for reading.", source);
        return false;
    }

    FILE *out = fopen(destination, "wb");

    if (NULL == out)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: failed to open destination '%s' for writing.",
                        destination);

        if (fclose(in) != 0)
        {
            POUND_LOG_WARN(&thread_logger, "fclose(source) also failed.");
        }

        return false;
    }

    char   buffer[FILE_BUFFER_SIZE] = { 0 };
    size_t bytes_read               = 0;
    size_t total_copied             = 0;
    bool   ok                       = true;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        if (ferror(in))
        {
            POUND_LOG_ERROR(
                &thread_logger, "Read error on source '%s' after %zu bytes.", source, total_copied);
            ok = false;
            break;
        }

        if (fwrite(buffer, 1, bytes_read, out) != bytes_read)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Write error on destination '%s' at offset %zu.",
                            destination,
                            total_copied);
            ok = false;
            break;
        }

        total_copied += bytes_read;

        if (total_copied > (size_t)1024 * 1024 * 1024)
        {
            POUND_LOG_ERROR(
                &thread_logger, "File '%s' exceeds 1 GiB sanity limit, aborting copy.", source);
            ok = false;
            break;
        }
    }

    if (ferror(in))
    {
        POUND_LOG_ERROR(&thread_logger, "Read error on source '%s'.", source);
        ok = false;
    }

    if (fclose(in) != 0)
    {
        POUND_LOG_WARN(&thread_logger, "fclose(source) failed for '%s'.", source);
        ok = false;
    }

    if (fflush(out) != 0)
    {
        POUND_LOG_ERROR(&thread_logger, "fflush failed for '%s'.", destination);
        ok = false;
    }

    if (fclose(out) != 0)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "fclose(destination) failed for '%s', data may be incomplete.",
                        destination);
        ok = false;
    }

    if (ok)
    {
        POUND_LOG_TRACE(&thread_logger,
                        "Copied %zu bytes from '%s' to '%s'.",
                        total_copied,
                        source,
                        destination);
    }
    else
    {
        POUND_LOG_WARN(&thread_logger,
                       "Incomplete copy from '%s' to '%s' (%zu bytes transferred).",
                       source,
                       destination,
                       total_copied);
        remove(destination);
    }

    return ok;
}

static void *
gui_create(const void *POUND_RESTRICT saved_state, size_t saved_size)
{
    if (saved_size > 0 && NULL == saved_state)
    {
        POUND_LOG_WARN(&thread_logger,
                       "saved_size is %zu but saved_state is NULL, ignoring saved state.",
                       saved_size);
        saved_state = NULL;
        saved_size  = 0;
    }

    gui_state_t *POUND_RESTRICT gui_state = calloc(1, sizeof(*gui_state));

    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: calloc failed for gui_state_t (%zu bytes).",
                        sizeof(gui_state_t));
        return NULL;
    }

    gui_state->show_demo             = false;
    gui_state->show_hot_reload_panel = true;

    if (saved_state && saved_size >= sizeof(*gui_state))
    {
        const gui_state_t *POUND_RESTRICT saved = saved_state;
        gui_state->show_demo                    = saved->show_demo;
        gui_state->show_hot_reload_panel        = saved->show_hot_reload_panel;
        gui_state->selected_tab                 = saved->selected_tab;
    }
    else if (saved_state != NULL && saved_size < sizeof(*gui_state))
    {
        POUND_LOG_WARN(&thread_logger,
                       "saved_size (%zu) < sizeof(gui_state_t) (%zu), "
                       "using defaults.",
                       saved_size,
                       sizeof(gui_state_t));
    }
    else
    {
        POUND_LOG_DEBUG(&thread_logger, "No saved state, using defaults.");
    }

    return gui_state;
}

void
gui_destroy(void *gui_state)
{
    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return;
    }

    free(gui_state);
}

void
gui_render_frame(void *gui_state)
{
    if (POUND_UNLIKELY(NULL == gui_state))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return;
    }

    gui_state_t *state = gui_state;

    const ImVec2_c debug_menu_size     = { .x = 270, .y = 150 };
    const ImVec2_c debug_menu_position = { .x = 1, .y = 1 };
    const ImVec2_c debug_menu_pivot    = { .x = 0, .y = 0 };
    igSetNextWindowPos(debug_menu_position, ImGuiCond_Always, debug_menu_pivot);
    igSetNextWindowSize(debug_menu_size, ImGuiCond_Always);
    igBegin("Debug Menu", NULL, 0);
    igCheckbox("Show Hot Reloading Guide", &state->show_hot_reload_panel);
    igCheckbox("Show ImGui Demo", &state->show_demo);

    if (state->show_hot_reload_panel)
    {
        igSeparator();
        igText("Rebuild PoundGui to relaod GUI code.");
        igText("Press F5 to force reload.");
    }

    if (state->show_demo)
    {
        igShowDemoWindow(NULL);
    }

    igEnd();
}

size_t
gui_save(void *gui_state, void *out, size_t capacity)
{
    if (NULL == gui_state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: gui_state is NULL.");
        return 0;
    }

    const gui_state_t *state = gui_state;

    if (NULL == out)
    {
        POUND_LOG_DEBUG(
            &thread_logger, "out is NULL, returning required size (%zu).", sizeof(*state));
        return sizeof(*state);
    }

    if (capacity < sizeof(*state))
    {
        POUND_LOG_WARN(&thread_logger,
                       "capacity (%zu) < sizeof(gui_state_t) (%zu), "
                       "cannot save state.",
                       capacity,
                       sizeof(*state));
        return sizeof(*state);
    }

    memcpy(out, state, sizeof(*state));
    return sizeof(*state);
}

void *
shared_library_load(const char *path)
{
    if (NULL == path)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: path is NULL.");
        return NULL;
    }

    if ('\0' == path[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: path is empty.");
        return NULL;
    }

    void *handle = NULL;

#if POUND_PLATFORM_WINDOWS

    handle = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (NULL == handle)
    {
        DWORD err = GetLastError();
        POUND_LOG_ERROR(&thread_logger,
                        "shared_library_load: LoadLibraryExA failed for '%s' (Win32 error %lu).",
                        path,
                        (unsigned long)err);
    }
#else

    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (NULL == handle)
    {
        const char *dl_error = dlerror();
        POUND_LOG_ERROR(&thread_logger,
                        "shared_library_load: dlopen failed for '%s': %s",
                        path,
                        dl_error ? dl_error : "unknown error");
    }

#endif // POUND_PLATFORM_WINDOWS

    return handle;
}

void
shared_library_unload(void *module)
{
    if (NULL == module)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: module is NULL.");
        return;
    }

#if POUND_PLATFORM_WINDOWS

    if (!FreeLibrary((HMODULE)module))
    {
        DWORD err = GetLastError();
        POUND_LOG_ERROR(&thread_logger,
                        "shared_library_unload: FreeLibrary failed (Win32 error %lu).",
                        (unsigned long)err);
    }

#else

    if (dlclose(module) != 0)
    {
        const char *dl_error = dlerror();
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: dlclose failed: %s",
                        dl_error ? dl_error : "unknown error");
    }

#endif // POUND_PLATFORM_WINDOWS
}

bool
shared_library_get_symbol(void *module, const char *name, void **out)
{
    if (NULL == module)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: module is NULL.");
        return false;
    }

    if (NULL == name)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: name is NULL.");
        return false;
    }

    if ('\0' == name[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: name is empty.");
        return false;
    }

    if (NULL == out)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out is NULL.");
        return false;
    }

    *out = NULL;

#if POUND_PLATFORM_WINDOWS

    FARPROC proc = GetProcAddress((HMODULE)module, name);

    if (!proc)
    {
        DWORD err = GetLastError();
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: GetProcAddress('%s') failed (Win32 error %lu).",
                        name,
                        (unsigned long)err);
        return false;
    }

    *out = (void *)proc;
    return true;

#else

    // Clear any stale error.
    dlerror();

    void *POUND_RESTRICT symbol       = dlsym(module, name);
    const char          *error_string = dlerror();

    if (NULL != error_string)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: dlsym('%s') error: %s", name, error_string);
        return false;
    }

    if (NULL == symbol)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: dlsym('%s') returned NULL without error.", name);
        return false;
    }

    *out = symbol;

#endif // POUND_PLATFORM_WINDOWS

    return true;
}

bool
shared_library_create_loaded_path(const char    *source,
                                  char          *destination,
                                  const size_t   capacity,
                                  const uint32_t id)
{
    if (NULL == source)
    {
        return false;
    }

    if (NULL == destination)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: destination is NULL.");
        return false;
    }

    if (0 == capacity)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: capacity is 0.");
        return false;
    }

    if ('\0' == source[0])
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: source is empty.");
        return false;
    }

    if (capacity < 16)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: capacity (%zu) too small to hold any valid path.",
                        capacity);
        return false;
    }

    if (0 == id)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: id is 0, hot_counter was not incremented before "
                        "calling this function.");
        return false;
    }

    const size_t source_length = strlen(source);

    if (source_length >= MAX_PATH)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: source exceeds MAX_PATH (%d).", source_length);
        return false;
    }

    const char *last_separator = strrchr(source, '/');
    const char *back_separator = strrchr(source, '\\');

    if (back_separator > last_separator)
    {
        last_separator = back_separator;
    }

    const char *dot = strrchr(source, '.');

    int written;

    if (dot && dot > last_separator)
    {
        const int stem_length = (int)(dot - source);

        if (stem_length < 0 || (size_t)stem_length > source_length)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: computed stem_length (%d) is invalid.",
                            stem_length);
            return false;
        }

        written = snprintf(destination, capacity, "%.*s.hot_%u%s", stem_length, source, id, dot);
    }
    else
    {
        written = snprintf(destination, capacity, "%s.hot_%u", source, id);
    }

    if (written < 0)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: snprintf returned negative (%d).", written);
        destination[0] = '\0';
        return false;
    }

    if ((size_t)written >= capacity)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: snprintf truncated output (needed %d, capacity %zu).",
                        written,
                        capacity);
        destination[0] = '\0';
        return false;
    }

    return true;
}

/*** end of file ***/
