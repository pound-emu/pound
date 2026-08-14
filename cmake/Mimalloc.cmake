include_guard(GLOBAL)

set(MIMALLOC_ROOT "${PROJECT_SOURCE_DIR}/extern/mimalloc")

if (NOT EXISTS "${MIMALLOC_ROOT}/include/mimalloc.h")
    message(FATAL_ERROR "[Mimalloc] Cannot find mimalloc, missing mimalloc.h header.")
endif ()

message(STATUS "[Mimalloc] Configuring mimalloc allocator...")
add_library(Mimalloc::Mimalloc SHARED IMPORTED GLOBAL)

set_target_properties(Mimalloc::Mimalloc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MIMALLOC_ROOT}/include"
)

if (WIN32)
    set_target_properties(Mimalloc::Mimalloc PROPERTIES
            IMPORTED_IMPLIB "${MIMALLOC_ROOT}/windows/lib/mimalloc.dll.lib"
            IMPORTED_LOCATION "${MIMALLOC_ROOT}/windows/bin/mimalloc.dll"
            IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
            IMPORTED_IMPLIB_RELEASE "${MIMALLOC_ROOT}/windows/lib/mimalloc.dll.lib"
            IMPORTED_LOCATION_RELEASE "${MIMALLOC_ROOT}/windows/bin/mimalloc.dll"
            IMPORTED_IMPLIB_DEBUG "${MIMALLOC_ROOT}/windows/lib/mimalloc-debug.dll.lib"
            IMPORTED_LOCATION_DEBUG "${MIMALLOC_ROOT}/windows/bin/mimalloc-debug.dll"
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            MAP_IMPORTED_CONFIG_MINSIZEREL Release
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(Mimalloc::Mimalloc PROPERTIES
            IMPORTED_LOCATION "${MIMALLOC_ROOT}/linux/lib/libmimalloc.so"
            IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
            IMPORTED_LOCATION_RELEASE "${MIMALLOC_ROOT}/linux/lib/libmimalloc.so"
            IMPORTED_LOCATION_DEBUG "${MIMALLOC_ROOT}/linux/lib/libmimalloc-debug.so"
            IMPORTED_NO_SONAME TRUE
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            MAP_IMPORTED_CONFIG_MINSIZEREL Release
    )
else ()
    message(FATAL_ERROR "[Mimalloc] mimalloc integration does not support this platform yet.")
endif ()