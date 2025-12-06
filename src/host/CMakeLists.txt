add_library(host STATIC)

target_sources(host PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/arena.c
)

target_link_libraries(host PRIVATE common)

target_include_directories(host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
