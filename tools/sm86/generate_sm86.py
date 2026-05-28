import argparse
import re
from collections import defaultdict
from typing import Any, Tuple

DEFAULT_SM70_ENCODE_RS_PATH = "sm70_encode.txt"
DEFAULT_OUTPUT_DIRECTORY = "../../src/gpu/sm86/generated/"


def classify_opcode(opcode_name: str) -> Tuple[str, int, int, int]:
    """ Returns: (class_enum, has_destination, num_inputs, is_memory)"""
    class_enum: str = "SM86_CLASS_UNKNOWN"
    has_destination: int = 1
    num_inputs: int = 2
    is_memory: int = 0
    FLOAT_ALU: set[str] = {"FADD", "FFMA", "FMNMX", "FMUL", "FSET", "FSETP", "FSWZADD", "MUFU", "DADD", "DFMA", "DMUL",
                           "DSETP",
                           "F2F", "F2I", "I2F", "FRND"}
    HALF_FLOAT_ALU: set[str] = {"HADD2", "HFMA2", "HMMA", "HMNMX2", "HMUL2", "HSET2", "HSETP2", "F2FP"}
    MEMORY_OPS: set[str] = {"LD", "LDC", "LDSM", "LDTRAM", "ST", "ATOM", "REDUX", "MEMBAR", "CCTL", "ALD", "AST"}
    TEXTURE_OPS: set[str] = {"TEX", "TLD", "TLD4", "TMML", "TXD", "TXQ"}
    SURFACE_OPS: set[str] = {"SULD", "SUST", "SUATOM"}
    CONTROL_FLOW: set[str] = {"BRA", "BREAK", "BSSY", "BSYNC", "EXIT", "KILL"}
    SYNC_OPS: set[str] = {"BAR", "WARPSYNC"}

    if opcode_name in FLOAT_ALU:
        class_enum = "SM86_CLASS_FLOAT_ALU"

        if opcode_name in {"FFMA", "DFMA"}:
            num_inputs = 3

        if opcode_name in {"FSETP", "DSETP"}:
            has_destination = 0
    elif opcode_name in HALF_FLOAT_ALU:
        class_enum = "SM86_CLASS_HALF_FLOAT_ALU"

        if opcode_name in {"HFMA2", "HMMA"}:
            num_inputs = 3

        if opcode_name == "HSETP2":
            has_destination = 0
    elif opcode_name in MEMORY_OPS:
        class_enum = "SM86_CLASS_MEMORY_LOAD_STORE"
        is_memory = 1

        if opcode_name in {"ST", "MEMBAR", "CCTL"}:
            has_destination = 0
    elif opcode_name in TEXTURE_OPS:
        class_enum = "SM86_CLASS_TEXTURE_FETCH"
        is_memory = 1
    elif opcode_name in SURFACE_OPS:
        class_enum = "SM86_CLASS_SURFACE_ATOMIC"
        is_memory = 1

        if opcode_name == "SUST":
            has_destination = 0
    elif opcode_name in CONTROL_FLOW:
        class_enum = "SM86_CLASS_CONTROL_FLOW"
        has_destination = 0
        num_inputs = 0
    elif opcode_name in SYNC_OPS:
        class_enum = "SM86_CLASS_SYNC_AND_YIELD"
        has_destination = 0
        num_inputs = 0
    else:
        class_enum = "SM86_CLASS_INT_ALU"

        if opcode_name in {"IADD3", "IADD3X", "IMAD", "IMAD64", "LOP3", "PLOP3"}:
            num_inputs = 3

        if opcode_name in {"ISETP"}:
            has_destination = 0

    return class_enum, has_destination, num_inputs, is_memory


if __name__ == "__main__":
    parser: argparse.ArgumentParser = argparse.ArgumentParser(description="Generate SM86 definitions")
    parser.add_argument("--sm70-encode", help="File path to the sm70_encode.rs file (found in mesa)")
    parser.add_argument("--output-directory", help="Directory to store the generated files")
    args: argparse.Namespace = parser.parse_args()
    sm70_file_path: str = DEFAULT_SM70_ENCODE_RS_PATH

    if args.sm70_encode is not None:
        sm70_file_path = args.sm70_encode

    output_directory: str = DEFAULT_OUTPUT_DIRECTORY

    if args.output_directory is not None:
        output_directory = args.output_directory

    with open(sm70_file_path, "r") as file:
        content: str = file.read()

    chunks = content.split("impl SM70Op for Op")

    # Regex to find "e.encode_alu(0x021)", etc.
    hex_pattern: re.Pattern[str] = re.compile(
        r'(?:encode_alu|encode_fp16v2_alu|encode_ualu|encode_alu_base|set_opcode)\s*\(\s*0x([0-9a-fA-F]+)')

    opcodes: defaultdict[Any, set[int]] = defaultdict(set)

    for chunk in chunks[1:]:
        name_match: re.Match[str] = re.match(r'([A-Za-z0-9_]+)', chunk)

        if not name_match:
            continue

        opcode_name: str = name_match.group(1).upper()
        hex_matches: list[str] = hex_pattern.findall(chunk)

        for hex_str in hex_matches:
            hex_value: int = int(hex_str, 16)
            opcodes[opcode_name].add(hex_value)

    c_header: str = "//! GENERATED_FILE - DO NOT EDIT\n"
    c_header += "//! Generated with tools/sm86/generate_sm86.py\n\n"
    c_header += "#ifndef POUND_GPU_SM86_OPCODES_H\n"
    c_header += "#define POUND_GPU_SM86_OPCODES_H\n\n"
    c_header += "#include \"stdint.h\"\n\n"
    c_header += "typedef enum : uint16_t\n{\n"
    c_header += "    SM86_OPCODE_NOP = 0,\n"

    for opcode_name in sorted(opcodes.keys()):
        if opcode_name == "NOP":
            continue

        c_header += f"    SM86_OPCODE_{opcode_name},\n"

    c_header += "   SM86_OPCODE_MAX_INSTRUCTIONS\n"
    c_header += "} sm86_opcode_t;\n\n"
    c_header += "extern const sm86_opcode_t g_sm86_opcodes_bits_to_enum[4096]; \n\n"
    c_header += "#endif // POUND_GPU_SM86_OPCODES_H\n\n"
    c_header += "/*** end of file ***/\n\n"
    c_source: str = "//! GENERATED_FILE - DO NOT EDIT\n"
    c_source += "//! Generated with tools/sm86/generate_sm86.py\n\n"
    c_source += "#include \"gpu/sm86/decoder.h\"\n"
    c_source += "#include \"gpu/sm86/generated/opcodes.h\"\n\n"
    c_source += "// Maps the opcode bits to its enum.\n"
    c_source += "const sm86_opcode_t g_sm86_opcodes_bits_to_enum[4096] =\n{\n"
    lut: list[str] = ["    SM86_OPCODE_NOP"] * 4096

    for opcode_name, hex_values in opcodes.items():
        for hex_value in hex_values:
            lut[hex_value] = f"    SM86_OPCODE_{opcode_name}"

    for i in range(4096):
        if lut[i] != "    SM86_OPCODE_NOP":
            c_source += f"    [0x{i:03x}] = {lut[i]},\n"

    c_source += "};\n\n"
    c_source += "// Maps the opcode enum to its metadata\n"
    c_source += "const sm86_instruction_metadata_t g_sm86_opcode_metadata[SM86_OPCODE_MAX_INSTRUCTIONS] =\n{\n"
    c_source += f"    [SM86_OPCODE_NOP] = {{ SM86_CLASS_UNKNOWN, 0, 0, 0}},\n"

    for opcode_name in sorted(opcodes.keys()):
        if opcode_name == "NOP":
            continue

        class_name, has_destination, num_inputs, is_memory = classify_opcode(opcode_name)
        c_source += f"    [SM86_OPCODE_{opcode_name}] ={{ {class_name}, {has_destination}, {num_inputs}, {is_memory} }},\n"

    c_source += "};\n\n"
    c_source += "/*** end of file ***/\n\n"
    c_header_path: str = output_directory + "opcodes.h"
    c_source_path: str = output_directory + "opcodes.c"

    with open(c_header_path, "w") as file:
        file.write(c_header)

    with open(c_source_path, "w") as file:
        file.write(c_source)
