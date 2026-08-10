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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFlags)

# -----------------------------------------------------------------------------
# Setup Build Directory
# -----------------------------------------------------------------------------

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(POUND_PLATFORM "windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(POUND_PLATFORM "mac")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(POUND_PLATFORM "linux")
else ()
    set(POUND_PLATFORM "Unknown")
endif ()

set(POUND_BINARIES_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${POUND_PLATFORM}")
set(POUND_LIBRARIES_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${POUND_PLATFORM}")

foreach (CONFIG IN ITEMS "" _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY${CONFIG} "${POUND_BINARIES_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY${CONFIG} "${POUND_LIBRARIES_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY${CONFIG} "${POUND_LIBRARIES_DIRECTORY}")
endforeach ()


message(STATUS "==================================================================")
message(STATUS "[POUND] Configuring Pound Emulator v${PROJECT_VERSION}")
message(STATUS "[POUND] Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "[POUND] Target OS:  ${CMAKE_SYSTEM_NAME}")
message(STATUS "[POUND] Compiler:   ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "[POUND] Binaries:   ${POUND_BINARIES_DIRECTORY}")
message(STATUS "[POUND] Libraries:  ${POUND_LIBRARIES_DIRECTORY}")
message(STATUS "==================================================================")

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

add_library(${PROJECT_NAME} STATIC
        src/gpu/sm86/decoder.c
        src/gpu/sm86/generated/opcodes.c
)
target_include_directories(${PROJECT_NAME} PRIVATE src/)
add_compiler_flags(${PROJECT_NAME})
add_linker_flags(${PROJECT_NAME})
add_sanitizers(${PROJECT_NAME})