find_package(Python3 REQUIRED)

# Define the generated files
set(GEN_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/decoder/arm32_table_generated.c)
set(GEN_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/decoder/arm32_table_generated.h)
set(INC_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/decoder/arm32.inc)
set(SCRIPT     ${CMAKE_SOURCE_DIR}/scripts/generate_jit_decoder_a32_table.py)

add_custom_command(
    OUTPUT ${GEN_SOURCE} ${GEN_HEADER}
    COMMAND Python3::Interpreter ${SCRIPT} ${INC_FILE} ${GEN_SOURCE} ${GEN_HEADER}
    DEPENDS ${SCRIPT} ${INC_FILE}
    COMMENT "Generating ARM32 Decoder Tables (Safety Compliance)"
)

add_library(jit STATIC)

target_sources(jit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/arm32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/arm32_table_generated.c
    #${CMAKE_CURRENT_SOURCE_DIR}/ir/type.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/ir/value.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/ir/opcode.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/ir/instruction.cpp
)

target_link_libraries(jit PRIVATE common)

target_include_directories(jit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
