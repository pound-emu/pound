# Define all the files that will be generated
set(GEN_OPCODES_H ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_opcodes.h)
set(GEN_DECODER_C ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_table.c)
set(GEN_DECODER_H ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_table.h)
set(GEN_HANDLER_TABLE ${CMAKE_CURRENT_SOURCE_DIR}/interpreter/arm32/handler_table.inc)
set(GEN_HANDLER_SKELETONS ${CMAKE_CURRENT_SOURCE_DIR}/interpreter/arm32/handlers.inc.skeleton)
set(INSTRUCTIONS ${CMAKE_CURRENT_SOURCE_DIR}/common/a32_instructions.inc)
set(SCRIPT ${CMAKE_SOURCE_DIR}/scripts/generate_jit_assets.py)

add_custom_command(
    OUTPUT ${GEN_OPCODES_H} ${GEN_DECODER_C} ${GEN_DECODER_H} ${GEN_HANDLER_TABLE} ${GEN_HANDLER_SKELETONS}
    COMMAND Python3::Interpreter ${SCRIPT} ${INSTRUCTIONS}
            --out-opcodes-h=${GEN_OPCODES_H}
            --out-decoder-c=${GEN_DECODER_C}
            --out-decoder-h=${GEN_DECODER_H}
            --out-handler-table-inc=${GEN_HANDLER_TABLE}
            --out-handler-skeletons-inc=${GEN_HANDLER_SKELETONS}
            DEPENDS ${SCRIPT} ${INSTRUCTIONS}
    COMMENT "Generating JIT assets"
)
add_library(jit STATIC)

target_sources(jit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/decoder/arm32_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/interpreter/arm32/instruction.c
    ${CMAKE_CURRENT_SOURCE_DIR}/interpreter/arm32/translator.c
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

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/interpreter/arm32/instruction.c
    PROPERTIES COMPILE_FLAGS "-Wno-gnu-label-as-value -Wno-unused-label -Wno-pedantic"
)

target_link_libraries(jit PRIVATE common host)

target_include_directories(jit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
