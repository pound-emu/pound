# Main Engine State

```c
#define REGISTER_FILE_SIZE 65536

#define ALIGN alignas(64)

 typedef struct
{
    ALIGN uint32_t 3d_registers[REGISTER_FILE_SIZE];
    ALIGN uint32_t compute_registers[REGISTER_FILE_SIZE];
    
    // GPU will communicate with the CPU via this ring buffer.
    //
    ALIGN uint64_t fifo_base_addres;
    uint32_t fifo_cpu_head;
    uint32_t fifo_gpu_head;
    
    // SMMU
    //
    ALIGN uint32_t *page_table;
} gpu_engine_t
```   

# SM86 Instructions

```c
// A raw 128-bit SM86 SASS instruction.
//
typedef struct
{
    uint64_t low;
    uint64_t high;
} sm86_raw_instruction_t


// SM86 SASS Encoding
// Ref: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/nouveau/compiler/nak/sm70_encode.rs?ref_type=heads
typedef struct
{
    uint16_t opcode;
    uint8_t destination_register;
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
    uint8_t form : 8;
    
    uint8_t source0_register;
    uint8_t source1_register;
    uint8_t source2_register;
    
    uint8_t source0_neg : 1; // -src0
    uint8_t source0_abs : 1; // |src0|
    uint8_t source1_neg : 1; // -src1
    uint8_t source1_abs : 1; // |src1|
    uint8_t saturate    : 1; // .SAT (Clamp to 0.0 - 1.0)
    uint8_t ftz         : 1; // .FTZ (Flush to zero)
    uint8_t is_uniform  : 1; // True if Uniform ALU (UGPR)
    uint8_t reserved    : 1;
    
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
            uint8_t binding_index;
            uint8_t padding;
        } cbuf;
    }
    
    // Extracted from bits 105..126 in set_instr_deps().
    //
    uint8_t delay_cycles;    // Cycles to stall before executing.
    uint8_t yield_flag;      // True if warp can yield to scheduler.
    uint8_t read_barrier;    // Wait on read barrier index.
    uint8_t write_barrier;   // Set write barrier index.
} sm86_decoded_instruction_t;

static_assert(sizeof(sm_86_decoded_instruction_t) == 16, "Struct must be 16 bytes or less");
```