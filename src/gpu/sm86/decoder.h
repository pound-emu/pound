#ifndef POUND_GPU_SM86_DECODER_H
#define POUND_GPU_SM86_DECODER_H

#include "generated/opcodes.h"
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // A raw 128-bit SM86 SASS instruction.
    //
    typedef struct
    {
        uint64_t low;
        uint64_t high;
    } sm86_raw_instruction_t;

    // SM86 SASS Encoding
    // Ref:
    // https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/nouveau/compiler/nak/sm70_encode.rs?ref_type=heads
    typedef struct
    {
        uint16_t opcode;
        uint8_t  destination_register;
        uint8_t  source0_register;
        uint8_t  source1_register;
        uint8_t  source2_register;

        uint8_t predicate_register : 3;
        uint8_t predicate_not : 1;

        // Src1:
        //      1 = Reg
        //      6 = UReg
        //      4 = Imm32
        //      5 = CBuf
        //
        // Src2:
        //      7 = UReg
        //      2 = Imm32
        //      3 = CBuf
        uint8_t form : 4;

    uint8_t source0_neg : 1; // -src0
    uint8_t source0_abs : 1; // |src0|
    uint8_t source1_neg : 1; // -src1
    uint8_t source1_abs : 1; // |src1|
    uint8_t saturate : 1;    // .SAT (Clamp to 0.0 - 1.0)
    uint8_t ftz : 1;         // .FTZ (Flush to zero)
    uint8_t is_uniform : 1;  // True if Uniform ALU (UGPR)
    uint8_t cmp_type : 1;
        uint8_t source0_neg : 1; // -src0
        uint8_t source0_abs : 1; // |src0|
        uint8_t source1_neg : 1; // -src1
        uint8_t source1_abs : 1; // |src1|
        uint8_t saturate : 1;    // .SAT (Clamp to 0.0 - 1.0)
        uint8_t ftz : 1;         // .FTZ (Flush to zero)
        uint8_t is_uniform : 1;  // True if Uniform ALU (UGPR)

    uint8_t  delay_cycles : 4;  // Cycles to stall before executing.
    uint8_t  yield_flag : 1;    // True if warp can yield to scheduler.
    uint8_t  read_barrier : 3;  // Wait on read barrier index.
    uint8_t  write_barrier : 3; // Set write barrier index.
    uint16_t cmp_operator : 3;
    uint16_t bool_operator : 2;
        uint8_t  delay_cycles : 4;  // Cycles to stall before executing.
        uint8_t  yield_flag : 1;    // True if warp can yield to scheduler.
        uint8_t  read_barrier : 3;  // Wait on read barrier index.
        uint8_t  write_barrier : 3; // Set write barrier index.

    uint16_t accumulator_predicate : 3;
    uint16_t accumulator_predicate_not : 1;
    uint16_t padding : 12;

        // An instruction cannot use an immediate and a constant buffer at the same time.
        //
        union
        {
            // form == 4.
            //
            int32_t immediate_value;

        // form == 5.
        //
        struct
        {
            uint16_t byte_offset;
            uint8_t  binding_index;
            uint8_t  padding;
        } constant_buffer;
    } payload;

            // form == 5.
            //
            struct
            {
                uint16_t byte_offset;
                uint8_t  binding_index;
                uint8_t  padding;
            } constant_buffer;
        } payload;

    } sm86_decoded_instruction_t;

    static_assert(sizeof(sm86_decoded_instruction_t) == 16, "Struct must be 16 bytes or less");

    typedef enum : uint8_t
    {
        SM86_CLASS_UNKNOWN = 0,
        SM86_CLASS_FLOAT_ALU,
        SM86_CLASS_INT_ALU,
        SM86_CLASS_HALF_FLOAT_ALU,
        SM86_CLASS_MEMORY_LOAD_STORE,
        SM86_CLASS_TEXTURE_FETCH,
        SM86_CLASS_SURFACE_ATOMIC,
        SM86_CLASS_CONTROL_FLOW,
        SM86_CLASS_SYNC_AND_YIELD,
    } sm86_instruction_class_t;

    typedef struct
    {
        sm86_instruction_class_t instruction_class;
        uint8_t                  has_destination;
        uint8_t                  num_inputs;
        uint8_t                  is_memory_instruction;
    } sm86_instruction_metadata_t;

    extern const sm86_instruction_metadata_t g_sm86_opcode_metadata[SM86_OPCODE_MAX_INSTRUCTIONS];

    void sm86_decode(const sm86_raw_instruction_t *raw_instruction,
                     sm86_decoded_instruction_t   *out_instruction);

#ifdef __cplusplus
}
#endif

#endif // POUND_GPU_SM86_DECODER_H

/*** end of file ***/