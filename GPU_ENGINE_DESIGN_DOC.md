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
} sm86_decoded_instruction_t;
```