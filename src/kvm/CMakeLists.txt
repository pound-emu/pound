add_library(kvm STATIC)

target_sources(kvm PRIVATE
    mmu.cpp
    mmio.cpp
    kvm.cpp
    guest.cpp
)

target_link_libraries(kvm PRIVATE common host)

target_include_directories(kvm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
