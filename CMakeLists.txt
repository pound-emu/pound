cmake_minimum_required(VERSION 3.22)

#------------------------
# ---- Project Setup ----
#------------------------

project(Pound)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-------------------------------
# ---- Dependency Discovery ----
#-------------------------------

find_package(OpenGL REQUIRED)
find_package(fmt 10.2.1 CONFIG)
find_package(SDL3 3.2.10 CONFIG)

message(STATUS "Verifying Git submodules integrity...")

# Check if submodules are properly initialized.
execute_process(
    COMMAND git submodule status
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE submodule_status
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to get submodule status")
endif()

# Function to verify a specific submodule's pinned commit
function(verify_pinned_commit name path actual_commit expected_commit)
    if(NOT actual_commit STREQUAL expected_commit)
        message(FATAL_ERROR
            "${name} submodule commit mismatch!\n"
            "Path: ${path}\n"
            "Expected: ${expected_commit}\n"
            "Actual:   ${actual_commit}\n"
            "Submodule may have been updated. Please revert to pinned commit with:\n"
            "  cd ${path}\n"
            "  git checkout ${expected_commit}\n"
            "  cd ../..\n"
            "  git add ${path}\n"
            "  git commit -m \"Revert ${name} to pinned commit\""
        )
    endif()

    message(STATUS "${name} commit matches pinned version: ${expected_commit}")
endfunction()

# Parse submodule status and verify each one
string(REPLACE "\n" ";" submodule_lines ${submodule_status})
foreach(line ${submodule_lines})
    # Extract status character (first character)
    string(SUBSTRING ${line} 0 1 status_char)

    # Extract commit hash
    string(REGEX REPLACE "^([ +-])([0-9a-f]+) .*" "\\2" commit_hash ${line})

    # Extract path
    string(REGEX REPLACE "^[ +-][0-9a-f]+ ([^ ]+).*" "\\1" submodule_path ${line})

    if(status_char STREQUAL "-")
        message(FATAL_ERROR "Submodule ${submodule_path} is not initialized. Please run: git submodule update --init --recursive")
    elseif(status_char STREQUAL "+")
        message(WARNING "Submodule ${submodule_path} has uncommitted chamges")
    endif()
       # Verify pinned commit for known submodules
        if(submodule_path STREQUAL "3rd_Party/imgui")
            verify_pinned_commit("imgui" "${submodule_path}" "${commit_hash}" "bf75bfec48fc00f532af8926130b70c0e26eb099")
        elseif(submodule_path STREQUAL "3rd_Party/SDL3")
            verify_pinned_commit("SDL3" "${submodule_path}" "${commit_hash}" "a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03")
        endif()

        message(STATUS "Verified submodule: ${submodule_path} (${commit_hash})")
endforeach()

message(STATUS "All submodules verified successfully")

#-----------------------------
# ---- Target Definitions ----
#-----------------------------

add_executable(Pound
    src/main.cpp
)

add_subdirectory(3rd_Party)
add_subdirectory(src/common)
add_subdirectory(src/frontend)
add_subdirectory(src/host)
add_subdirectory(src/jit)
add_subdirectory(src/pvm)
add_subdirectory(src/targets/switch1/hardware)

#--------------------------------
# ---- Target Configurations ----
#--------------------------------

if(WIN32)
    add_compile_options(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

include(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

list(APPEND POUND_PROJECT_TARGETS common frontend host pvm)
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

    # Set Compile time log level for all targets.
    # 1: Trace
    # 2: Debug
    # 3: Info
    # 4: Warning
    # 5: Error
    # 6: Fatal
    target_compile_definitions(${TARGET} PRIVATE COMPILE_TIME_LOG_LEVEL=1)
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
    jit
    pvm

    OpenGL::GL
    SDL3::SDL3
    imgui
)
