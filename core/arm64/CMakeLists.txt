#Copyright 2025 Pound Emulator Project.All rights reserved.

set(ARM64_SOURCES ${CMAKE_CURRENT_SOURCE_DIR} / isa.cpp ${CMAKE_CURRENT_SOURCE_DIR} /
    guest.cpp ${CMAKE_CURRENT_SOURCE_DIR} / mmu.cpp)

    target_sources(Pound PRIVATE ${ARM64_SOURCES})

        target_include_directories(Pound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} /..)
