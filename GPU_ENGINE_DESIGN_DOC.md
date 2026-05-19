```c
#define REGISTER_FILE_SIZE 65536

typedef struct
{
    uint32_t 3d_registers[REGISTER_FILE_SIZE];
    uint32_t compute_registers[REGISTER_FILE_SIZE]
} gpu_engine_t
```                                                     