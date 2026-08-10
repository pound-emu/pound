include_guard(GLOBAL)
set(BALLISTIC_ROOT "${PROJECT_SOURCE_DIR}/extern/ballistic")

if (NOT EXISTS "${BALLISTIC_ROOT}/include/bal_engine.h")
    message(FATAL_ERROR "[Ballistic] Cannot find Ballistic, missing bal_engine.h header.")
endif ()

message(STATUS "[Ballistic] Configuring Ballistic JIT Engine...")

add_library(Ballistic::Engine STATIC IMPORTED GLOBAL)
set_target_properties(Ballistic::Engine PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BALLISTIC_ROOT}/include")

if (WIN32)
    set_target_properties(Ballistic::Engine PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ROOT}/windows/lib/Ballistic.lib"
    )
    add_library(Ballistic::LuaJIT SHARED IMPORTED GLOBAL)
    set_target_properties(Ballistic::LuaJIT PROPERTIES
            IMPORTED_IMPLIB "${BALLISTIC_ROOT}/windows/lib/lua51.lib"
            IMPORTED_LOCATION "${BALLISTIC_ROOT}/windows/bin/lua51.dll"
    )
    target_link_libraries(Ballistic::Engine INTERFACE Ballistic::LuaJIT)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(Ballistic::Engine PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ROOT}/linux/lib/libBallistic.a"
    )
    add_library(Ballistic::LuaJIT SHARED IMPORTED GLOBAL)
    set_target_properties(Ballistic::LuaJIT PROPERTIES
            IMPORTED_LOCATION "${BALLISTIC_ROOT}/linux/bin/libluajit.so"
            IMPORTED_NO_SONAME TRUE
    )
    target_link_libraries(Ballistic::Engine INTERFACE Ballistic::LuaJIT)
else ()
    message(FATAL_ERROR "[Ballistic] Ballistic integration does not support this platform yet.")
endif ()