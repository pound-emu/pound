#define LOG_MODULE "main"

#include "common/logging.h"
#include "common/passert.h"
#include "host/memory/arena.h"
#include "jit/frontend/decoder/arm32.h"
#include "jit/interpreter/arm32/translator.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SYSTEM_MEMORY_SIZE    (64 * 1024 * 1024) // 64 MB
#define MAX_TRANSLATION_LIMIT 4096

#define EXIT_CODE_SUCCESS       0
#define EXIT_CODE_ERROR_GENERAL 1
#define EXIT_CODE_ERROR_IO      2
#define EXIT_CODE_ERROR_MEMORY  3

static const char *const DEFAULT_ROM_PATH = "../src/test_long_alu_pc_write.bin";

static int load_binary_file(const char              *filename,
                            pvm_host_memory_arena_t *arena,
                            uint32_t               **out_ptr,
                            size_t                  *out_size);

int
main (int argc, char **argv)
{
    int         exit_code   = EXIT_CODE_SUCCESS;
    const char *target_file = DEFAULT_ROM_PATH;

    if (argc > 1)
    {
        target_file = argv[1];
    }
    else
    {
        LOG_INFO("No input file specified. Defaulting to: %s",
                 DEFAULT_ROM_PATH);
    }

    pvm_host_memory_arena_t arena
        = pvm_host_memory_arena_init(SYSTEM_MEMORY_SIZE);
    if (NULL == arena.data)
    {
        LOG_FATAL("Failed to initalize system memory arena (%d bytes)",
                  SYSTEM_MEMORY_SIZE);
        exit_code = EXIT_CODE_ERROR_MEMORY;
        goto cleanup;
    }

    LOG_INFO("System memory initialized: %d MB",
             SYSTEM_MEMORY_SIZE / (1024 * 1024));

    uint32_t *guest_code            = NULL;
    size_t    guest_code_size_bytes = 0;
    if (0
        != load_binary_file(
            target_file, &arena, &guest_code, &guest_code_size_bytes))
    {
        LOG_FATAL("Failed to load guest binary. Aborting.");
        exit_code = EXIT_CODE_ERROR_IO;
        goto cleanup;
    }

    LOG_INFO(
        "Loaded payload: %s (%zu bytes)", target_file, guest_code_size_bytes);

    size_t instruction_count = guest_code_size_bytes / sizeof(uint32_t);
    if (0 == instruction_count)
    {
        LOG_WARNING("File is empty or contains no full instructions.");
        goto cleanup;
    }

    /*
     * Cap the translation to the safety limit or the actual size,
     * whichever is smaller.
     */
    size_t instructions_to_process = (instruction_count < MAX_TRANSLATION_LIMIT)
                                         ? instruction_count
                                         : MAX_TRANSLATION_LIMIT;

    if (instruction_count > MAX_TRANSLATION_LIMIT)
    {
        LOG_WARNING(
            "Input exceeds safety limit. Only translating first %d "
            "instructions.",
            MAX_TRANSLATION_LIMIT);
    }

    /* Execute JIT translation */
    pvm_jit_interpreter_arm32_block_t            block = { 0 };
    pvm_jit_interpreter_arm32_translate_result_t result
        = pvm_jit_interpreter_arm32_translate(
            &arena, &block, guest_code, instructions_to_process);

    if (PVM_JIT_INTERPRETER_ARM32_TRANSLATE_SUCCESS != result)
    {
        LOG_ERROR("JIT Translation failed with error code: %d", result);

        switch (result)
        {
            case PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OUT_OF_MEMORY:
                LOG_ERROR("Cause: Out of Memory");
                break;
            case PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_OVERFLOW:
                LOG_ERROR("Cause: Integer Overflow in size calculation");
                break;
            case PVM_JIT_INTERPRETER_ARM32_TRANSLATE_ERROR_MEMORY_ALLOCATION_FAILED:
                LOG_ERROR("Cause: Internal Arena Allocation Failed");
                break;
            default:
                LOG_ERROR("Cause: Unknown");
                break;
        }

        exit_code = EXIT_CODE_ERROR_GENERAL;
        goto cleanup;
    }

    LOG_INFO("Translation Successfull");
    LOG_INFO("Generated %zu interpreter micro-ops", block.count);

    if (NULL == block.instructions)
    {
        LOG_FATAL("Logic Error: Translator returned Success but block instructions are NULL");
        exit_code = EXIT_CODE_ERROR_GENERAL;
        goto cleanup;
    }

    /* Print translated block */
    for (size_t i = 0; i < block.count; ++i)
    {
        const pvm_jit_decoder_arm32_instruction_info_t *info = pvm_jit_decoder_arm32_decode(block.instructions[i].raw);
        const char* instruction_name = "UNKNOWN";
        if (NULL != info)
        {
            instruction_name = info->name;
        }

        LOG_DEBUG("Op[%03zu]: %s (Raw: 0x%08X)", i, instruction_name, block.instructions[i].raw);
    }

cleanup:
    if (NULL != arena.data)
    {
        LOG_TRACE("Tearing down memory arena...");
        pvm_host_memory_arena_free(&arena);
    }

    if (EXIT_CODE_SUCCESS == exit_code)
    {
        LOG_INFO("Pound terminated successfully.");
    }
    else
    {
        LOG_ERROR("Pound terminated with error code: %d", exit_code);
    }
    return exit_code;
}

static int
load_binary_file (const char              *filename,
                  pvm_host_memory_arena_t *arena,
                  uint32_t               **out_ptr,
                  size_t                  *out_size)
{
    if ((NULL == filename) || (NULL == arena) || (NULL == out_ptr)
        || (NULL == out_size))
    {
        LOG_FATAL("Invalid arguments passed to load_binary_file");
        return -1;
    }

    FILE *file = fopen(filename, "rb");
    if (NULL == file)
    {
        LOG_ERROR("Failed to open file: %s", filename);
        return -1;
    }

    long offset = 0;
    if (0 != fseek(file, offset, SEEK_END))
    {
        LOG_ERROR("fseek failed on file: %s", filename);
        goto cleanup_file;
    }

    long file_size_signed = ftell(file);
    if (file_size_signed < 0)
    {
        LOG_ERROR("ftell returned invalid size for file: %s", filename);
        goto cleanup_file;
    }

    rewind(file);
    size_t file_size = (size_t)file_size_signed;
    if ((file_size % sizeof(uint32_t)) != 0)
    {
        LOG_WARNING(
            "File size (%zu) is not a multiple of 4. Instructions may be "
            "truncated.",
            file_size);
    }

    size_t arena_available_space = arena->capacity - arena->size;
    if (file_size > arena_available_space)
    {
        LOG_FATAL("Insufficient memory in arena. Required %zu, Available: %zu",
                  file_size,
                  arena_available_space);
        goto cleanup_file;
    }

    void *data = pvm_host_memory_arena_allocate(arena, file_size);
    if (NULL == data)
    {
        LOG_FATAL("Arena allocation returned NULL despite capacity check.");
        goto cleanup_file;
    }

    if (((uintptr_t)data % _Alignof(uint32_t)) != 0)
    {
        LOG_FATAL(
            "Arena allocator returned misaligned memory. Cannot cast to "
            "uint32_t* safely..");
        goto cleanup_file;
    }

    size_t bytes_read = fread(data, 1, file_size, file);
    if (bytes_read != file_size)
    {
        LOG_ERROR("Incomplete file read. Expected %zu bytes, got %zu.",
                  file_size,
                  bytes_read);
        goto cleanup_file;
    }

    *out_ptr  = (uint32_t *)data;
    *out_size = file_size;

cleanup_file:
    if (NULL != file)
    {
        (void)fclose(file);
    }
    return 0;
}
