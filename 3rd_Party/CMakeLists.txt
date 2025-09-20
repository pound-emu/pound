set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON SYSTEM ON)

# Set CMP0069 policy to "NEW" for building external targets with LTO enabled
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

# fmt
if (NOT TARGET fmt::fmt)
  add_subdirectory(fmt)
endif()

# SDL3
if (NOT TARGET SDL3::SDL3)
  set(SDL_DISKAUDIO OFF)
  set(SDL_TEST_LIBRARY OFF)
  set(SDL_PIPEWIRE OFF)
  add_subdirectory(SDL3)
endif()

# ImGui
set(IMGUI_SRC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SRC})
target_link_libraries(imgui PRIVATE SDL3::SDL3)
target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
)
