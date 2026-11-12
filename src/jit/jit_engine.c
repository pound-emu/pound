#include "jit_engine.h"
#include "log.h"
#include <string.h>

/// Maximum guest instructions Unicorn can execute before returning control to Ballistic.
/// Keep this small to minimize state divergence.
#define JIT_FALLBACK_INSTRUCTION_LIMIT 64

jit_engine_t g_jit_engine = {};

static const int UNICORN_REGISTER_BATCH_IDS[32] = {
    UC_ARM64_REG_X0,  UC_ARM64_REG_X1,  UC_ARM64_REG_X2,  UC_ARM64_REG_X3,  UC_ARM64_REG_X4,
    UC_ARM64_REG_X5,  UC_ARM64_REG_X6,  UC_ARM64_REG_X7,  UC_ARM64_REG_X8,  UC_ARM64_REG_X9,
    UC_ARM64_REG_X10, UC_ARM64_REG_X11, UC_ARM64_REG_X12, UC_ARM64_REG_X13, UC_ARM64_REG_X14,
    UC_ARM64_REG_X15, UC_ARM64_REG_X16, UC_ARM64_REG_X17, UC_ARM64_REG_X18, UC_ARM64_REG_X19,
    UC_ARM64_REG_X20, UC_ARM64_REG_X21, UC_ARM64_REG_X22, UC_ARM64_REG_X23, UC_ARM64_REG_X24,
    UC_ARM64_REG_X25, UC_ARM64_REG_X26, UC_ARM64_REG_X27, UC_ARM64_REG_X28, UC_ARM64_REG_X29,
    UC_ARM64_REG_X30, UC_ARM64_REG_PC,
};

#define UNICORN_REGISTER_BATCH_COUNT 32

static void           engine_thread_entry(void *POUND_RESTRICT engine);
static pound_error_t  engine_run(jit_engine_t *POUND_RESTRICT engine);
static const uint8_t *ballistic_translate_guest_address(void               *context,
                                                        bal_guest_address_t guest_address,
                                                        size_t             *max_readable);

POUND_COLD static pound_error_t unicorn_init(jit_engine_t *POUND_RESTRICT engine);
POUND_COLD static void          unicorn_destroy(jit_engine_t *POUND_RESTRICT engine);
POUND_HOT static pound_error_t  unicorn_load_state_from_ballistic(
    jit_engine_t *POUND_RESTRICT engine);
POUND_HOT static pound_error_t unicorn_save_state_to_ballistic(jit_engine_t *POUND_RESTRICT engine);
POUND_HOT static pound_error_t unicorn_run(jit_engine_t *POUND_RESTRICT engine,
                                           uint64_t                    *out_instructions_executed);

pound_error_t
jit_engine_init(jit_engine_t *context, guest_memory_t *guest_memory, const uint32_t flags)
{
    if (POUND_UNLIKELY(NULL == context))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: context is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(NULL == guest_memory))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: guest_memory is NULL.");
        context->status = POUND_ERROR_INVALID_ARGUMENT;
        return context->status;
    }

    if (POUND_UNLIKELY(NULL == guest_memory->host_base))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: guest_memory->host_base is NULL.");
        context->status = POUND_ERROR_INVALID_ARGUMENT;
        return context->status;
    }

    if (POUND_UNLIKELY(0 == guest_memory->host_size))
    {
        POUND_LOG_ERROR(&thread_logger, "guest_memory->host_size is 0");
        context->status = POUND_ERROR_INVALID_ARGUMENT;
        return context->status;
    }

    POUND_LOG_DEBUG(&thread_logger, "Configuring JIT subsystem...");
    (void)memset(context, 0, sizeof(jit_engine_t));
    context->guest_memory   = guest_memory;
    context->engine_flags   = flags;
    context->stop_requested = false;

    memset(&context->ballistic_cpu, 0, sizeof(bal_cpu_t));
    context->ballistic_memory_interface.context   = guest_memory;
    context->ballistic_memory_interface.translate = ballistic_translate_guest_address;

    POUND_LOG_DEBUG(&thread_logger,
                    "Memory interface bridge configured: "
                    "guest range=[0x%llx, 0x%llx), host_base=%p.",
                    (unsigned long long)guest_memory->guest_base,
                    (unsigned long long)guest_memory->guest_end,
                    (void *)guest_memory->host_base);

    bal_allocator_default_init(&context->allocator);

    const bal_error_t ballistic_error = bal_engine_init(&context->ballistic_engine,
                                                        &context->ballistic_cpu,
                                                        &context->allocator,
                                                        &context->ballistic_memory_interface);

    if (POUND_UNLIKELY(ballistic_error != BAL_SUCCESS))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: Ballistic failed to initialize.");
        context->status = POUND_ERROR_INVALID_ARGUMENT;
        return context->status;
    }

    context->ballistic_engine.flags = flags;
    context->initialized            = true;
    context->status                 = POUND_SUCCESS;
    POUND_LOG_INFO(&thread_logger, "Successfully configured JIT subsystem.");
    return context->status;
}

POUND_HOT pound_error_t
jit_engine_start(jit_engine_t *POUND_RESTRICT engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(false == engine->initialized))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: engine not initialized. Call "
                        "jit_engine_init() first.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(true == engine->thread_active))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "JIT thread is already running. Call jit_engine_stop() + "
                        "jit_engine_join() first.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    const size_t            stack_size   = 2U * 1024U * 1024U; // 2 KiB
    const thread_priority_t priority     = THREAD_PRIORITY_ABOVE_NORMAL;
    const uint32_t          affinity     = 0U;
    const pound_error_t     thread_error = thread_create(engine_thread_entry,
                                                         engine,
                                                         stack_size,
                                                         priority,
                                                         "PoundJIT",
                                                         affinity,
                                                         &engine->thread_id);

    if (thread_error != POUND_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger, "thread_create() failed with error %d.", thread_error);
        engine->thread_active = false;
        engine->status        = thread_error;
        return engine->status;
    }

    engine->thread_active = true;

    POUND_LOG_INFO(&thread_logger,
                   "JIT thread spawned. stack=%zu bytes, priority=%d.",
                   stack_size,
                   (int)priority);
    return POUND_SUCCESS;
}

bool
jit_engine_join(jit_engine_t *POUND_RESTRICT engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(THREAD_ID_INVALID == engine->thread_id))
    {
        POUND_LOG_WARN(&thread_logger, "No thread to join. Was jit_engine_start() called?");
        return true;
    }

    POUND_LOG_DEBUG(&thread_logger, "Waiting for JIT thread...");
    const bool joined = thread_join(engine->thread_id);

    if (POUND_UNLIKELY(false == joined))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "thread_join failed. Thread may have been detached or "
                        "already exited.");
    }
    else
    {
        POUND_LOG_DEBUG(&thread_logger, "JIT thread joined successfully.");
    }

    engine->thread_id     = THREAD_ID_INVALID;
    engine->thread_active = false;
    return joined;
}

bool
jit_engine_is_running(const jit_engine_t *engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        return false;
    }

    return engine->thread_active;
}

POUND_HOT void
jit_engine_stop(jit_engine_t *POUND_RESTRICT engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        return;
    }

    POUND_LOG_DEBUG(&thread_logger, "Stop requested.");
    engine->stop_requested = true;
    bal_engine_stop_thread(&engine->ballistic_engine);

    if (engine->unicorn_initialized && NULL != engine->unicorn_engine)
    {
        uc_emu_stop(engine->unicorn_engine);
    }
}

POUND_COLD pound_error_t
jit_engine_reset(jit_engine_t *POUND_RESTRICT engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(false == engine->initialized))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is not initialized.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    const bal_error_t error = bal_engine_reset(&engine->ballistic_engine);

    if (POUND_UNLIKELY(error != BAL_SUCCESS))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: bal_engine_reset() failed because "
                        "%s",
                        bal_error_to_string(error));
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    engine->stop_requested = false;
    engine->status         = POUND_SUCCESS;
    POUND_LOG_INFO(&thread_logger, "JIT engine reset complete.");
    return POUND_SUCCESS;
}

POUND_COLD void
jit_engine_destroy(jit_engine_t *engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        return;
    }

    if (POUND_UNLIKELY(false == engine->initialized))
    {
        POUND_LOG_WARN(&thread_logger, "JIT engine was never initialized, nothing to destory.");
        return;
    }

    POUND_LOG_INFO(&thread_logger,
                   "Destroying JIT engine. stats: ballistic=%llu, unicorn=%llu, "
                   "fallbacks=%llu, resumes=%llu.",
                   (unsigned long long)engine->stats.ballistic_instructions,
                   (unsigned long long)engine->stats.unicorn_instructions,
                   (unsigned long long)engine->stats.fallback_count,
                   (unsigned long long)engine->stats.resume_count);

    unicorn_destroy(engine);
    bal_engine_destroy(&engine->ballistic_engine);
    (void)memset(engine, 0, sizeof(jit_engine_t));
    POUND_LOG_DEBUG(&thread_logger, "JIT engine destoryed.");
}

POUND_HOT jit_stats_t
jit_engine_get_stats(const jit_engine_t *engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        const jit_stats_t empty = { 0 };
        return empty;
    }

    return engine->stats;
}

void
engine_thread_entry(void *engine)
{
    pound_logger_init_default();

    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: engine is NULL.");
        return;
    }

    jit_engine_t *POUND_RESTRICT jit_engine = engine;
    POUND_LOG_INFO(&thread_logger,
                   "JIT thread started. PC=0x%llx.",
                   (unsigned long long)jit_engine->ballistic_cpu.pc);

    const pound_error_t result = engine_run(jit_engine);

    if (POUND_SUCCESS != result)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "JIT thread exiting with error %d. pc=0x%llx.",
                        (int)result,
                        (unsigned long long)jit_engine->ballistic_cpu.pc);
    }
    else
    {
        POUND_LOG_INFO(&thread_logger,
                       "JIT thread exiting cleanly. PC=0x%llX.",
                       (unsigned long long)jit_engine->ballistic_cpu.pc);
    }

    jit_engine->thread_active = false;
}

pound_error_t
engine_run(jit_engine_t *engine)
{
    if (POUND_UNLIKELY(NULL == engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: context is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (POUND_UNLIKELY(false == engine->initialized))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: engine not initialized, call "
                        "jit_engine_init() first.");
        engine->status = POUND_ERROR_INVALID_ARGUMENT;
        return engine->status;
    }

    POUND_LOG_INFO(&thread_logger,
                   "Starting JIT engine at PC 0x%llx.",
                   (unsigned long long)engine->ballistic_cpu.pc);

    const bool *POUND_RESTRICT   stop_requested   = &engine->stop_requested;
    bal_engine_t *POUND_RESTRICT ballistic_engine = &engine->ballistic_engine;
    bal_cpu_t *POUND_RESTRICT    ballistic_cpu    = &engine->ballistic_cpu;
    jit_stats_t *POUND_RESTRICT  jit_stats        = &engine->stats;

    while (false == (int)*stop_requested)
    {
        const bal_error_t ballistic_result = bal_engine_run_thread(ballistic_engine);
        jit_stats->ballistic_instructions += ballistic_cpu->instruction_count;

        if (POUND_LIKELY(BAL_SUCCESS == ballistic_result))
        {
            POUND_LOG_INFO(
                &thread_logger,
                "Ballistic exited cleanly. PC = 0x%llX, total ballistic instructions=%llu"
                ".",
                (unsigned long long)ballistic_cpu->pc,
                (unsigned long long)jit_stats->ballistic_instructions);
            engine->status = POUND_SUCCESS;
            return engine->status;
        }

        if (BAL_ERROR_UNKNOWN_INSTRUCTION == ballistic_result)
        {
            ++jit_stats->fallback_count;
            const uint64_t fault_pc = ballistic_cpu->pc;

            POUND_LOG_DEBUG(
                &thread_logger,
                "FALLBACK #%llu: Unknown instruction at PC=0x%llX, delegating to Unicorn"
                ".",
                (unsigned long long)jit_stats->fallback_count,
                (unsigned long long)fault_pc);

            if (POUND_UNLIKELY(false == engine->unicorn_initialized))
            {
                const pound_error_t unicorn_init_error = unicorn_init(engine);

                if (POUND_UNLIKELY(false == engine->unicorn_initialized))
                {
                    POUND_LOG_ERROR(&thread_logger,
                                    "Aborting function: Unicorn init failed at "
                                    "PC=0x%llX.",
                                    (unsigned long long)fault_pc);
                    engine->status = unicorn_init_error;
                    return engine->status;
                }
            }

            const pound_error_t unicorn_load_state_error
                = unicorn_load_state_from_ballistic(engine);

            if (POUND_UNLIKELY(unicorn_load_state_error != POUND_SUCCESS))
            {
                POUND_LOG_ERROR(&thread_logger,
                                "Aborting execution: failed to load state from "
                                "Ballistic to Unicorn at PC=0x%llX.",
                                (unsigned long long)fault_pc);
                engine->status = unicorn_load_state_error;
                return engine->status;
            }

            uint64_t            unicorn_instructions = 0U;
            const pound_error_t unicorn_run_error    = unicorn_run(engine, &unicorn_instructions);

            if (POUND_UNLIKELY(unicorn_run_error != POUND_SUCCESS))
            {
                POUND_LOG_ERROR(&thread_logger,
                                "Aborting function: Unicorn failed at PC=0x%llX.",
                                (unsigned long long)fault_pc);
                engine->status = unicorn_run_error;
                return engine->status;
            }

            jit_stats->unicorn_instructions += unicorn_instructions;
            const pound_error_t unicorn_save_state_error = unicorn_save_state_to_ballistic(engine);

            if (POUND_UNLIKELY(unicorn_save_state_error != POUND_SUCCESS))
            {
                POUND_LOG_ERROR(&thread_logger,
                                "Aborting function: failed to save Unicorn state "
                                "to Ballistic. PC=0x%llX.",
                                (unsigned long long)ballistic_cpu->pc);
                engine->status = unicorn_save_state_error;
                return engine->status;
            }

            ++jit_stats->resume_count;
            POUND_LOG_DEBUG(&thread_logger,
                            "RESUME #%llu: Unicorn executed %llu instructions. "
                            "Resuming Ballistic at PC=0x%llX.",
                            (unsigned long long)jit_stats->resume_count,
                            (unsigned long long)unicorn_instructions,
                            (unsigned long long)ballistic_cpu->pc);
            bal_engine_clear_cache(ballistic_engine);
        }

        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: Ballistic returned fatal error "
                        "because %s at PC=0x%llX.",
                        bal_error_to_string(ballistic_result),
                        (unsigned long long)ballistic_cpu->pc);
        engine->status = POUND_ERROR_MEMORY_FAULT;
        return engine->status;
    }
}

static const uint8_t *
ballistic_translate_guest_address(void                     *context,
                                  const bal_guest_address_t guest_address,
                                  size_t                   *max_readable)
{
    if (POUND_UNLIKELY(NULL == context))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: context is NULL.");
        return NULL;
    }

    if (POUND_UNLIKELY(NULL == max_readable))
    {
        POUND_LOG_ERROR(&thread_logger, "max_readable is NULL.");
        return NULL;
    }

    const guest_memory_t *POUND_RESTRICT guest_memory = context;
    const uint8_t *result = guest_memory_translate_read(guest_memory, guest_address, max_readable);

    if (POUND_UNLIKELY(NULL == result))
    {
        POUND_LOG_WARN(&thread_logger,
                       "jit_dual_translate_bridge: guest address 0x%llx is unmapped. "
                       "guest_memory range [0x%llx, 0x%llx).",
                       (unsigned long long)guest_address,
                       (unsigned long long)guest_memory->guest_base,
                       (unsigned long long)guest_memory->guest_end);
    }

    return result;
}

static pound_error_t
unicorn_init(jit_engine_t *restrict engine)
{
    POUND_LOG_DEBUG(&thread_logger, "Initializing Unicorn engine...");
    uc_engine   *unicorn_engine     = NULL;
    const uc_err unicorn_open_error = uc_open(UC_ARCH_ARM64, UC_MODE_ARM, &unicorn_engine);

    if (unicorn_open_error != UC_ERR_OK)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_open() failed: %s.",
                        uc_strerror(unicorn_open_error));
        return POUND_ERROR_MEMORY_FAULT;
    }

    const uint64_t guest_base = engine->guest_memory->guest_base;
    const size_t   host_size  = engine->guest_memory->host_size;
    void          *host_base  = engine->guest_memory->host_base;

    const uc_err unicorn_memory_map_ptr_error
        = uc_mem_map_ptr(unicorn_engine, guest_base, host_size, UC_PROT_ALL, host_base);

    if (unicorn_memory_map_ptr_error != UC_ERR_OK)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_mem_map_ptr() failed: %s. "
                        "guest_base=0x%llX, size=%zu.",
                        uc_strerror(unicorn_memory_map_ptr_error),
                        guest_base,
                        host_size);
        uc_close(unicorn_engine);
        return POUND_ERROR_MEMORY_FAULT;
    }

    engine->unicorn_engine      = unicorn_engine;
    engine->unicorn_initialized = true;
    POUND_LOG_INFO(&thread_logger,
                   "Unicorn initialized. Mapping %zu bytes at guest_base=0x%llX"
                   ".",
                   host_size,
                   (unsigned long long)guest_base);
    return POUND_SUCCESS;
}

POUND_COLD static void
unicorn_destroy(jit_engine_t *POUND_RESTRICT engine)
{
    if (NULL == engine->unicorn_engine)
    {
        return;
    }

    POUND_LOG_DEBUG(&thread_logger, "Destroying Unicorn engine.");
    uc_close(engine->unicorn_engine);
    engine->unicorn_engine      = NULL;
    engine->unicorn_initialized = false;
}

pound_error_t
unicorn_load_state_from_ballistic(jit_engine_t *restrict engine)
{
    uc_engine *POUND_RESTRICT unicorn_engine = engine->unicorn_engine;

    if (POUND_UNLIKELY(NULL == unicorn_engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: Unicorn engine is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    const bal_cpu_t *POUND_RESTRICT ballistic_cpu = &engine->ballistic_cpu;
    const uc_err batch_error = uc_reg_write_batch(unicorn_engine,
                                                  UNICORN_REGISTER_BATCH_IDS,
                                                  (const void *const *)&ballistic_cpu->x[0],
                                                  UNICORN_REGISTER_BATCH_COUNT);

    if (POUND_UNLIKELY(batch_error != UC_ERR_OK))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_reg_write_batch() failed: %s.",
                        uc_strerror(batch_error));
        return POUND_ERROR_MEMORY_FAULT;
    }

    uint32_t nzcv = 0U;

    if (ballistic_cpu->flag_n)
    {
        nzcv |= 1U << 31;
    }

    if (ballistic_cpu->flag_z)
    {
        nzcv |= 1U << 30;
    }

    if (ballistic_cpu->flag_c)
    {
        nzcv |= 1U << 29;
    }

    if (ballistic_cpu->flag_v)
    {
        nzcv |= 1U << 28;
    }

    const uc_err flag_write_error = uc_reg_write(unicorn_engine, UC_ARM64_REG_NZCV, &nzcv);

    if (POUND_UNLIKELY(flag_write_error != UC_ERR_OK))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_reg_write(NZCV) failed: %s.",
                        uc_strerror(flag_write_error));
        return POUND_ERROR_MEMORY_FAULT;
    }

    return POUND_SUCCESS;
}

pound_error_t
unicorn_save_state_to_ballistic(jit_engine_t *restrict engine)
{
    uc_engine *POUND_RESTRICT unicorn_engine = engine->unicorn_engine;

    if (POUND_UNLIKELY(NULL == unicorn_engine))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: Unicorn engine is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    bal_cpu_t *POUND_RESTRICT ballistic_cpu = &engine->ballistic_cpu;
    void                     *register_values[UNICORN_REGISTER_BATCH_COUNT] = { 0 };

    for (int i = 0; i < UNICORN_REGISTER_BATCH_COUNT; ++i)
    {
        register_values[i] = (void *)&ballistic_cpu->x[i];
    }

    const uc_err read_batch_error = uc_reg_read_batch(
        unicorn_engine, UNICORN_REGISTER_BATCH_IDS, register_values, UNICORN_REGISTER_BATCH_COUNT);

    if (POUND_UNLIKELY(read_batch_error != UC_ERR_OK))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_reg_read_batch() failed: %s.",
                        uc_strerror(read_batch_error));
        return POUND_ERROR_MEMORY_FAULT;
    }

    uint32_t     nzcv       = 0U;
    const uc_err flag_error = uc_reg_read(unicorn_engine, UC_ARM64_REG_NZCV, &nzcv);

    if (POUND_UNLIKELY(UC_ERR_OK != flag_error))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_reg_read(NZCV) failed: %s.",
                        uc_strerror(flag_error));
        return POUND_ERROR_MEMORY_FAULT;
    }

    ballistic_cpu->flag_n = (uint8_t)((nzcv >> 31U) & 1U);
    ballistic_cpu->flag_z = (uint8_t)((nzcv >> 30U) & 1U);
    ballistic_cpu->flag_c = (uint8_t)((nzcv >> 29U) & 1U);
    ballistic_cpu->flag_v = (uint8_t)((nzcv >> 28U) & 1U);
    return POUND_SUCCESS;
}

static pound_error_t
unicorn_run(jit_engine_t *restrict engine, uint64_t *out_instructions_executed)
{
    if (POUND_UNLIKELY(NULL == out_instructions_executed))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: out_instructions_executed is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    uc_engine *POUND_RESTRICT unicorn_engine = engine->unicorn_engine;
    bal_cpu_t *POUND_RESTRICT ballistic_cpu  = &engine->ballistic_cpu;
    const uint64_t            start_pc       = ballistic_cpu->pc;
    const uint64_t            guest_end      = engine->guest_memory->guest_end;

    POUND_LOG_TRACE(&thread_logger,
                    "Starting Unicorn Engine, PC=0x%llX, max_instructions=%d.",
                    (unsigned long long)start_pc,
                    JIT_FALLBACK_INSTRUCTION_LIMIT);

    const uint64_t timeout   = 0U;
    const uc_err   run_error = uc_emu_start(
        unicorn_engine, start_pc, guest_end, timeout, JIT_FALLBACK_INSTRUCTION_LIMIT);

    if (run_error != UC_ERR_OK)
    {
        uint64_t stopped_pc = 0U;
        uc_reg_read(unicorn_engine, UC_ARM64_REG_PC, &stopped_pc);
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: uc_emu_start() failed: %s. "
                        "start_pc=0x%llX, stopped_pc=0x%llX.",
                        uc_strerror(run_error),
                        start_pc,
                        stopped_pc);
        *out_instructions_executed = 0U;
        return POUND_ERROR_MEMORY_FAULT;
    }

    uint64_t end_pc = 0;
    uc_reg_read(unicorn_engine, UC_ARM64_REG_PC, &end_pc);

    // Precise counting requires a hook which is expensive.
    *out_instructions_executed = JIT_FALLBACK_INSTRUCTION_LIMIT;
    ballistic_cpu->pc          = end_pc;
    POUND_LOG_TRACE(
        &thread_logger, "Unicorn run completed: end_pc=0x%llX.", (unsigned long long)end_pc);
    return POUND_SUCCESS;
}