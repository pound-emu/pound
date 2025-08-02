# Copyright 2025 Pound Emulator Project. All rights reserved.

# GUI sources
set(GUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/gui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/color.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/panels.cpp
)

# Add all GUI sources to the main target
target_sources(Pound PRIVATE
        ${GUI_SOURCES}
)

# Include directories for GUI
target_include_directories(Pound PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)