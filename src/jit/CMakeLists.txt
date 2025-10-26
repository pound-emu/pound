add_library(jit STATIC)

target_sources(jit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/arm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/type.cpp
)

target_link_libraries(jit PRIVATE common host)

target_include_directories(jit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
