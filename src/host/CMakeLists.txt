add_library(host STATIC)

target_sources(host PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/arena.cpp
)

target_include_directories(host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
