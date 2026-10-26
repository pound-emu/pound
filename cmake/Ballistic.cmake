include_guard(GLOBAL)

function(ballistic_verify_directory path description)
    string(STRIP "${path}" path)

    if (NOT IS_DIRECTORY "${path}")
        message(FATAL_ERROR "[Ballistic] Missing ${description}: ${path}")
    endif ()
endfunction()

function(ballistic_verify_file path description)
    string(STRIP "${path}" path)

    if (NOT EXISTS "${path}")
        message(FATAL_ERROR "[Ballistic] Missing ${description}: ${path}")
    endif ()

    if (IS_DIRECTORY "${path}")
        message(FATAL_ERROR "[Ballistic] ${description} is a directory, expected a file: ${path}")
    endif ()
endfunction()

function(ballistic_verify_imported_path target property)
    get_target_property(path ${target} ${property})

    if (NOT path)
        return()
    endif ()

    if (path STREQUAL "path-NOTFOUND")
        return()
    endif ()

    string(STRIP "${path}" path)

    if (NOT EXISTS "${path}")
        message(FATAL_ERROR "[Ballistic] ${target} ${property} does not exist: ${path}")
    endif ()

    if (IS_DIRECTORY "${path}")
        message(FATAL_ERROR "[Ballistic] ${target} ${property} is a directory: ${path}")
    endif ()

    set_target_properties(${target} PROPERTIES ${property} "${path}")
endfunction()

set(BALLISTIC_ROOT "${PROJECT_SOURCE_DIR}/extern/ballistic")

ballistic_verify_directory("${BALLISTIC_ROOT}" "Ballistic root directory")
ballistic_verify_directory("${BALLISTIC_ROOT}/include" "Ballistic include directory")
ballistic_verify_file("${BALLISTIC_ROOT}/include/bal_engine.h" "bal_engine.h header")

message(STATUS "[Ballistic] Configuring Ballistic JIT Engine...")

add_library(Ballistic::Engine STATIC IMPORTED GLOBAL)
set_target_properties(Ballistic::Engine PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BALLISTIC_ROOT}/include")

if (WIN32)
    set(BALLISTIC_ENGINE_LIBRARY "${BALLISTIC_ROOT}/windows/lib/Ballistic.lib")
    ballistic_verify_file("${BALLISTIC_ENGINE_LIBRARY}" "Ballistic import library")

    set_target_properties(Ballistic::Engine PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ENGINE_LIBRARY}"
    )

    add_library(Ballistic::LuaJIT SHARED IMPORTED GLOBAL)
    set_target_properties(Ballistic::LuaJIT PROPERTIES
            IMPORTED_IMPLIB "${BALLISTIC_ROOT}/windows/lib/lua51.lib"
            IMPORTED_LOCATION "${BALLISTIC_ROOT}/windows/bin/lua51.dll"
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BALLISTIC_ENGINE_LIBRARY "${BALLISTIC_ROOT}/linux/lib/libBallistic.a")

    ballistic_verify_file("${BALLISTIC_ENGINE_LIBRARY}" "Ballistic static library")

    set_target_properties(Ballistic::Engine PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ENGINE_LIBRARY}"
    )
else ()
    message(FATAL_ERROR "[Ballistic] Ballistic integration does not support this platform yet.")
endif ()

ballistic_verify_imported_path(Ballistic::Engine IMPORTED_LOCATION)

if (WIN32)
    ballistic_verify_imported_path(Ballistic::LuaJIT IMPORTED_IMPLIB)
endif ()
