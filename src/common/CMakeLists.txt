#Copyright 2025 Pound Emulator Project.All rights reserved.

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Assert.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IoFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PathUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging/Backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging/Filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging/TextFormatter.cpp)

target_sources(Pound PRIVATE ${COMMON_SOURCES})

target_include_directories(Pound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
