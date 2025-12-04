
# Define the generated files
set(GEN_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_table_generated.c)
set(GEN_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_table_generated.h)
set(INC_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32.inc)
set(SCRIPT     ${CMAKE_SOURCE_DIR}/scripts/generate_jit_decoder_a32_table.py)

add_custom_command(
    OUTPUT ${GEN_SOURCE} ${GEN_HEADER}
    COMMAND Python3::Interpreter ${SCRIPT} ${INC_FILE} ${GEN_SOURCE} ${GEN_HEADER}
    DEPENDS ${SCRIPT} ${INC_FILE}
    COMMENT "Generating ARM32 Decoder Tables"
)

add_library(jit STATIC)

target_sources(jit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_table_generated.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/type.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/value.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/opcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/instruction.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/basic_block.c
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/opcode.c
    PROPERTIES COMPILE_FLAGS "-Wno-c23-extensions -Wno-pedantic"
)

target_link_libraries(jit PRIVATE common host)

target_include_directories(jit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
