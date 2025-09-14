#Copyright 2025 Pound Emulator Project.All rights reserved.

set(KVM_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/mmu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mmio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kvm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/guest.cpp
)

target_sources(Pound PRIVATE ${KVM_SOURCES})

        target_include_directories(Pound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} /..)
