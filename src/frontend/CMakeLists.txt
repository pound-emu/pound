add_library(frontend STATIC)

target_sources(frontend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/color.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels.cpp
)

target_link_libraries(frontend PRIVATE common imgui SDL3::SDL3)

target_include_directories(frontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
