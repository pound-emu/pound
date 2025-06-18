# Copyright 2025 Xenon Emulator Project. All rights reserved.

cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Optimizations
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
if (WIN32)
  add_compile_options($<$<CONFIG:Release>:/Oi>)
  add_compile_options($<$<CONFIG:Release>:/Ot>)
endif()

project(Pound)

find_package(fmt 10.2.1 CONFIG)
find_package(toml11 4.4.0 CONFIG)
include_directories(3rd_Party/rem/src)

include_directories(core)
add_subdirectory(3rd_Party)


file(GLOB_RECURSE Core core/*.cpp core/*.h)

add_executable(Pound
  ${Core}
)

target_precompile_headers(Pound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core/Base/Types.h)

# Link libraries
target_link_libraries(Pound PRIVATE fmt::fmt toml11::toml11)

if (WIN32)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

  # Disables Warnings
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

  target_sources(Pound PRIVATE core/Pound.rc)
endif()
