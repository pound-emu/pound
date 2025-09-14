cmake_minimum_required(VERSION 3.22)

#------------------------
# ---- Project Setup ----
#------------------------

project(Pound)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Enable asserts in release mode. Windows throws a metric fuck ton of compiler
# errors when asserts are disabled.
#if (UNIX)
#   string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

#-------------------------------
# ---- Dependency Discovery ----
#-------------------------------

find_package(OpenGL REQUIRED)
find_package(fmt 10.2.1 CONFIG)
find_package(SDL3 3.2.10 CONFIG)

#-----------------------------
# ---- Target Definitions ----
#-----------------------------

if(WIN32)
    add_compile_options(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

add_executable(Pound
    src/main.cpp
)

add_subdirectory(3rd_Party)
add_subdirectory(src/common)
add_subdirectory(src/frontend)
add_subdirectory(src/host)
add_subdirectory(src/kvm)
add_subdirectory(src/targets/switch1/hardware)

#--------------------------------
# ---- Target Configurations ----
#--------------------------------

include(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

list(APPEND POUND_PROJECT_TARGETS common frontend host kvm)
foreach(TARGET ${POUND_PROJECT_TARGETS})
    # Apply Endianness definitions to all our targets.
    if(WORDS_BIGENDIAN)
        target_compile_definitions(${TARGET} PRIVATE HOST_IS_BIG_ENDIAN=1 HOST_IS_LITTLE_ENDIAN=0)
    else()
        target_compile_definitions(${TARGET} PRIVATE HOST_IS_BIG_ENDIAN=0 HOST_IS_LITTLE_ENDIAN=1)
    endif()
    target_compile_options(${TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall
        -Wpedantic
        -Wshadow
        -Wpointer-arith
        -Wcast-qual
        -Wcast-align
        -Wconversion>
    )
endforeach()



# Optimizations
set_property(TARGET Pound PROPERTY CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
if (WIN32)
    target_compile_options(Pound PRIVATE $<$<CONFIG:Release>:/Oi>)
    target_compile_options(Pound PRIVATE  $<$<CONFIG:Release>:/Ot>)
endif()

target_link_libraries(Pound PRIVATE
    common
    frontend
    host
    kvm

    OpenGL::GL
    fmt::fmt
    SDL3::SDL3
    imgui
)
