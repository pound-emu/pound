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

project(Pound LANGUAGES C VERSION 0.1.0)

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
message(STATUS "[Pound] Configuring Pound Emulator v${PROJECT_VERSION}")
message(STATUS "[Pound] Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "[Pound] Target OS:  ${CMAKE_SYSTEM_NAME}")
message(STATUS "[Pound] Compiler:   ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "[Pound] Binaries:   ${POUND_BINARIES_DIRECTORY}")
message(STATUS "[Pound] Libraries:  ${POUND_LIBRARIES_DIRECTORY}")
message(STATUS "==================================================================")


if (POUND_ENABLE_LINK_TIME_OPTIMIZATION)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if (ipo_supported)
        message(STATUS "[Pound] Link Time Optimization is ENABLED and supported.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else ()
        message(WARNING "[Pound] Link Time Optimization was requested but is NOT supported by your compiler, disabling LTO. "
                "Reason: ${ipo_error}")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    endif ()
else ()
    message(STATUS "[Pound] Link Time Optimization is DISABLED via CMake options.")
endif ()

# -----------------------------------------------------------------------------
# Compile Pound
# -----------------------------------------------------------------------------

include(Ballistic)
include(Mimalloc)

message(STATUS "[SDL] Configuring SDL library...")

# Build static SDL.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

# Bare minimum required for rendering.
set(SDL_VIDEO ON CACHE BOOL "" FORCE)
set(SDL_RENDER ON CACHE BOOL "" FORCE)
set(SDL_EVENTS ON CACHE BOOL "" FORCE)
set(SDL_THREADS ON CACHE BOOL "" FORCE)
set(SDL_TIMERS ON CACHE BOOL "" FORCE)
set(OFF CACHE BOOL "" FORCE)

# Disable everything nonessential.
foreach (sdl_option
        SDL_AUDIO
        SDL_JOYSTICK
        SDL_HAPTIC
        SDL_SENSOR
        SDL_POWER
        SDL_HIDAPI
        SDL_CAMERA
        SDL_GPU
        SDL_STORAGE
        SDL_DIALOG
        SDL_FILE
        SDL_FILESYSTEM
        SDL_LOCALE
        SDL_MISC
        SDL_ATOMIC
        SDL_CPUINFO
        SDL_X11_XSCRNSAVER
        SDL_X11_XTEST
)
    set(${sdl_option} OFF CACHE BOOL "" FORCE)
endforeach ()
add_subdirectory(extern/sdl/sdl)

# Suppress clang warnings.
set_target_properties(SDL3-static PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:SDL3::SDL3,INTERFACE_INCLUDE_DIRECTORIES>"
)

message(STATUS "[PoundCore] Configuring Pound Core Library...")
add_library(PoundCore STATIC
        src/core/guest_memory.c
        src/core/guest_state.c
)
target_include_directories(PoundCore PUBLIC src/core)
add_compiler_flags(PoundCore)
add_linker_flags(PoundCore)
add_sanitizers(PoundCore)

message(STATUS "[Pound] Configuring Pound Executable...")

add_executable(${PROJECT_NAME} src/main.c)
target_include_directories(${PROJECT_NAME} PRIVATE src/)
target_link_libraries(${PROJECT_NAME} PRIVATE PoundCore Ballistic::Engine Mimalloc::Mimalloc SDL3::SDL3)
add_compiler_flags(${PROJECT_NAME})
add_linker_flags(${PROJECT_NAME})
add_sanitizers(${PROJECT_NAME})

message(STATUS "==================================================================")
message(STATUS "[Pound] Cmake configuration completed successfully.")
message(STATUS "==================================================================")