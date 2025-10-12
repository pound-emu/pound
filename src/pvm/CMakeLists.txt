add_library(pvm STATIC)

target_sources(pvm PRIVATE
    mmu.cpp
    mmio.cpp
    pvm.cpp
    guest.cpp
)

target_link_libraries(pvm PRIVATE common host)

target_include_directories(pvm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
