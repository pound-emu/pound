cmake_minimum_required(VERSION 3.25)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF) # Strict ISO C

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

project(Pound LANGUAGES C VERSION 0.1.0)

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
            -Wvla
    )
endif ()

add_library(${PROJECT_NAME} STATIC
        src/gpu/sm86/decoder.c
        src/gpu/sm86/generated/opcodes.c
)
target_include_directories(${PROJECT_NAME} PRIVATE src/)


if (WIN32)
    # Disable some Windows SDK deprecation warnings
    target_compile_options(${PROJECT_NAME} PRIVATE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE ${POUND_COMPILER_OPTIONS_UNIX})
endif ()