cmake_minimum_required(VERSION 3.25)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF) # Strict ISO C
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

project(Pound LANGUAGES C CXX VERSION 0.1.0)

# -----------------------------------------------------------------------------
# Compile Pound
# -----------------------------------------------------------------------------

find_package(Python3 REQUIRED)

set(SCRIPT_GENERATE_SM86 ${CMAKE_SOURCE_DIR}/tools/sm86/generate_sm86.py)
set(SM70_FILE_PATH ${CMAKE_SOURCE_DIR}/tools/sm86/sm70_encode.txt)
set(OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src/gpu/sm86/generated/)

add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/src/gpu/sm86/generated/opcodes.h ${CMAKE_SOURCE_DIR}/src/gpu/sm86/generated/opcodes.c
        COMMAND Python3::Interpreter ${SCRIPT_GENERATE_SM86} --sm70-encode ${SM70_FILE_PATH} --output-directory ${OUTPUT_DIRECTORY}
        DEPENDS ${SCRIPT_GENERATE_SM86}
)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else ()
    set(POUND_COMPILER_OPTIONS_UNIX
            -Wall -Wextra -Werror
            -Wconversion -Wsign-conversion -Wshadow
            -Wstrict-prototypes -Wmissing-prototypes
            -Wno-gnu-zero-variadic-macro-arguments # Make formatting assert messages easier
            -fno-strict-aliasing # Prevent common C aliasing UB
            -fstack-protector-strong
            -Wpadded
            -Wvla
    )
endif ()

add_library(${PROJECT_NAME} STATIC
        src/gpu/recompiler/decoder.c
        src/gpu/recompiler/generated/opcodes.c
)
target_include_directories(${PROJECT_NAME} PRIVATE src/)


if (WIN32)
    # Disable some Windows SDK deprecation warnings
    target_compile_options(${PROJECT_NAME} PRIVATE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE ${POUND_COMPILER_OPTIONS_UNIX})
endif ()

# -----------------------------------------------------------------------------
# Compile Tests
# -----------------------------------------------------------------------------

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(extern/googletest-1.17.0)
target_compile_options(gtest PUBLIC -Wno-error -w)
include(GoogleTest)
enable_testing()
add_executable(test_engine tests/test_engine.cpp)
target_include_directories(test_engine PRIVATE src/ extern/googletest-1.17.0/googletest/include)
target_link_libraries(test_engine PRIVATE gtest gtest_main ${PROJECT_NAME})
gtest_discover_tests(test_engine)