# Copyright 2025 Pound Emulator Project. All rights reserved.

set(HOST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/arena.cpp
)

target_sources(Pound PRIVATE
    ${HOST_SOURCES}
)

target_include_directories(Pound PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
