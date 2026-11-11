#include "jit_engine.h"

#include "log.h"

#include <string.h>

static void           engine_thread_entry(void *POUND_RESTRICT engine);
static pound_error_t  engine_run(jit_engine_t *POUND_RESTRICT engine);
static const uint8_t *ballistic_translate_guest_address(void               *context,
                                                        bal_guest_address_t guest_address,
                                                        size_t             *max_readable);

POUND_COLD static void unicorn_destroy(jit_engine_t *POUND_RESTRICT engine);

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