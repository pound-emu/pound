add_library(common STATIC)

target_sources(common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/logging.c
    ${CMAKE_CURRENT_SOURCE_DIR}/passert.c
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
