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
    set(BALLISTIC_LUAJIT_IMPLIB "${BALLISTIC_ROOT}/windows/lib/lua51.lib")
    set(BALLISTIC_LUAJIT_RUNTIME "${BALLISTIC_ROOT}/windows/bin/lua51.dll")
    set(BALLISTIC_LUAJIT_RUNTIME_NAME "lua51.dll")

    ballistic_verify_file("${BALLISTIC_ENGINE_LIBRARY}" "Ballistic import library")
    ballistic_verify_file("${BALLISTIC_LUAJIT_IMPLIB}" "LuaJIT import library")
    ballistic_verify_file("${BALLISTIC_LUAJIT_RUNTIME}" "LuaJIT shared library")

    set_target_properties(Ballistic::Engine PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ENGINE_LIBRARY}"
    )

    add_library(Ballistic::LuaJIT SHARED IMPORTED GLOBAL)

    set_target_properties(Ballistic::LuaJIT PROPERTIES
            IMPORTED_IMPLIB "${BALLISTIC_LUAJIT_IMPLIB}"
            IMPORTED_LOCATION "${BALLISTIC_LUAJIT_RUNTIME}"
    )
    target_link_libraries(Ballistic::Engine INTERFACE Ballistic::LuaJIT)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BALLISTIC_ENGINE_LIBRARY "${BALLISTIC_ROOT}/linux/lib/libBallistic.a")
    set(BALLISTIC_LUAJIT_RUNTIME "${BALLISTIC_ROOT}/linux/bin/libluajit.so")
    # The bundled libluajit.so has SONAME libluajit-5.1.so.2, so that is the file name
    # the dynamic loader searches for at runtime, not libluajit.so.
    set(BALLISTIC_LUAJIT_RUNTIME_NAME "libluajit-5.1.so.2")

    ballistic_verify_file("${BALLISTIC_ENGINE_LIBRARY}" "Ballistic static library")
    ballistic_verify_file("${BALLISTIC_LUAJIT_RUNTIME}" "LuaJIT shared library")

    set_target_properties(Ballistic::Engine PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ENGINE_LIBRARY}"
    )

    add_library(Ballistic::LuaJIT SHARED IMPORTED GLOBAL)

    set_target_properties(Ballistic::LuaJIT PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_LUAJIT_RUNTIME}"
            IMPORTED_NO_SONAME TRUE
    )

    target_link_libraries(Ballistic::Engine INTERFACE Ballistic::LuaJIT)
else ()
    message(FATAL_ERROR "[Ballistic] Ballistic integration does not support this platform yet.")
endif ()

ballistic_verify_imported_path(Ballistic::Engine IMPORTED_LOCATION)
ballistic_verify_imported_path(Ballistic::LuaJIT IMPORTED_LOCATION)

if (WIN32)
    ballistic_verify_imported_path(Ballistic::LuaJIT IMPORTED_IMPLIB)
endif ()

# Copies the LuaJIT runtime next to the given executable so the loader can find it.
# Windows searches the executable's directory for DLLs, and on Linux the library has
# to exist under its SONAME, which the bundled file does not.
function(ballistic_deploy_runtime target)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${BALLISTIC_LUAJIT_RUNTIME}"
            "$<TARGET_FILE_DIR:${target}>/${BALLISTIC_LUAJIT_RUNTIME_NAME}"
            COMMENT "[Ballistic] Deploying LuaJIT runtime for ${target}"
            VERBATIM
    )
endfunction()
