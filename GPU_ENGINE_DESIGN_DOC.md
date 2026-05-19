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