# Copyright 2025 Pound Emulator Project. All rights reserved.

set(HARDWARE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/probe.cpp
)

target_sources(Pound PRIVATE
    ${HARDWARE_SOURCES}
)

target_include_directories(Pound PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
