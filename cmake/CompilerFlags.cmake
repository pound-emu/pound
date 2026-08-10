include_guard(GLOBAL)

# Macro to add compiler flags
function(add_compiler_flags target_name)
    if (DEFINED POUND_COMPILER_FLAGS AND NOT POUND_COMPILER_FLAGS STREQUAL "")
        separate_arguments(COMPILER_FLAGS_LIST NATIVE_COMMAND "${POUND_COMPILER_FLAGS}")
        target_compile_options(${target_name} PRIVATE ${COMPILER_FLAGS_LIST})
    endif ()
endfunction()

# Macro to add linker flags.
function(add_linker_flags target_name)
    if (DEFINED POUND_LINKER_FLAGS AND NOT POUND_LINKER_FLAGS STREQUAL "")
        separate_arguments(LINKER_FLAGS_LIST NATIVE_COMMAND "${POUND_LINKER_FLAGS}")
        target_link_options(${target_name} PRIVATE ${LINKER_FLAGS_LIST})
    endif ()
endfunction()

# Macro to add sanitizers.
function(add_sanitizers target_name)
    if (DEFINED POUND_SANITIZERS AND NOT POUND_SANITIZERS STREQUAL "")
        target_compile_options(${target_name} PRIVATE ${POUND_SANITIZERS})
        target_link_options(${target_name} PRIVATE ${POUND_SANITIZERS})
    endif ()
endfunction()