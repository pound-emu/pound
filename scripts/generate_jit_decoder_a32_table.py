#!/usr/bin/env python3
import re
import sys
import datetime
import argparse

# ---------------------------------------------------------
# Configuration & Logic
# ---------------------------------------------------------

MAX_BUCKET_SIZE = 18
TABLE_SIZE = 4096

class Instruction:
    def __init__(self, name, mnemonic, bitstring, array_index):
        self.name = name
        self.mnemonic = mnemonic
        self.bitstring = bitstring
        self.array_index = array_index
        self.mask = 0
        self.expected = 0
        self.parse_bits()

    def parse_bits(self):
        if len(self.bitstring) != 32:
            print(f"Error: Bitstring length {len(self.bitstring)} invalid for {self.name}")
            sys.exit(1)

        for i, char in enumerate(self.bitstring):
            bit_pos = 31 - i
            if char == '0':
                self.mask |= (1 << bit_pos)
            elif char == '1':
                self.mask |= (1 << bit_pos)
                self.expected |= (1 << bit_pos)

    def get_hash(self):
        major = (self.expected >> 20) & 0xFF
        minor = (self.expected >> 4) & 0x0F
        return (major << 4) | minor

def parse_inc_file(input_path):
    instructions = []
    regex = re.compile(r'INST\(\s*([A-Za-z0-9_]+),\s*"(.*?)",\s*"(.*?)"\s*\)')

    try:
        with open(input_path, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Error: Could not find input file: {input_path}")
        sys.exit(1)

    index_counter = 0
    for line in lines:
        line = line.strip()
        if not line or line.startswith("//"):
            continue

        match = regex.search(line)
        if match:
            inst = Instruction(match.group(1), match.group(2), match.group(3), index_counter)
            instructions.append(inst)
            index_counter += 1
    return instructions

def generate_lookup_table(instructions):
    buckets = {i: [] for i in range(TABLE_SIZE)}
    for inst in instructions:
        idx = inst.get_hash()
        buckets[idx].append(inst)
        if len(buckets[idx]) > MAX_BUCKET_SIZE:
            print(f"FATAL ERROR: Bucket {idx:#05x} overflowed! Size: {len(buckets[idx])}")
            sys.exit(1)
    return buckets

def write_c_file(path, instructions, buckets):
    with open(path, 'w') as f:
        f.write("/* GENERATED FILE - DO NOT EDIT */\n")
        f.write('#include "arm32.h"\n')
        f.write('#include "arm32_table_generated.h"\n')

        f.write(f"static const pvm_jit_decoder_arm32_instruction_info_t g_instructions[{len(instructions)}] = {{\n")
        for inst in instructions:
            f.write(f'    {{ "{inst.mnemonic}", "{inst.bitstring}", {inst.mask:#010x}U, {inst.expected:#010x}U }},\n')
        f.write("};\n\n")

        f.write(f"const decode_bucket_t g_decoder_lookup_table[{TABLE_SIZE}] = {{\n")
        for i in range(TABLE_SIZE):
            if len(buckets[i]) > 0:
                f.write(f"    [{i:#05x}] = {{ .instructions = {{ ")
                for inst in buckets[i]:
                    f.write(f"&g_instructions[{inst.array_index}], ")
                f.write(f"}}, .count = {len(buckets[i])}U }},\n")
        f.write("};\n")

def write_h_file(path):
    with open(path, 'w') as f:
        f.write("#ifndef POUND_JIT_DECODER_ARM32_GENERATED_H\n")
        f.write("#define POUND_JIT_DECODER_ARM32_GENERATED_H\n\n")
        f.write('#include "arm32.h"\n')
        f.write('#include <stddef.h>\n\n')
        f.write(f"#define LOOKUP_TABLE_MAX_BUCKET_SIZE {MAX_BUCKET_SIZE}U\n\n")
        f.write("typedef struct {\n")
        f.write("    const pvm_jit_decoder_arm32_instruction_info_t *instructions[LOOKUP_TABLE_MAX_BUCKET_SIZE];\n")
        f.write("    size_t count;\n")
        f.write("} decode_bucket_t;\n\n")
        f.write(f"extern const decode_bucket_t g_decoder_lookup_table[{TABLE_SIZE}];\n\n")
        f.write("#endif\n")

# ---------------------------------------------------------
# Main Execution
# ---------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Generate ARM32 Decoder Tables")
    parser.add_argument("input", help="Path to arm32.inc")
    parser.add_argument("out_c", help="Path to output .c file")
    parser.add_argument("out_h", help="Path to output .h file")
    args = parser.parse_args()

    print(f"--- Generating Decoder: {args.input} -> {args.out_c} ---")
    instructions = parse_inc_file(args.input)
    buckets = generate_lookup_table(instructions)
    write_c_file(args.out_c, instructions, buckets)
    write_h_file(args.out_h)

if __name__ == "__main__":
    main()
