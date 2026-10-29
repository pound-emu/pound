#include "jit_engine.h"

#include "log.h"

#include <string.h>

static const uint8_t *ballistic_translate_guest_address(void               *context,
                                                        bal_guest_address_t guest_address,
                                                        size_t             *max_readable);

POUND_COLD static void unicorn_destroy(jit_engine_t *POUND_RESTRICT engine);

error_t
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

    memset(&context->cpu, 0, sizeof(bal_cpu_t));
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
                                                        &context->cpu,
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

error_t
jit_engine_run(jit_engine_t *engine)
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

    POUND_LOG_INFO(
        &thread_logger, "Starting JIT engine at PC 0x%llx.", (unsigned long long)engine->cpu.pc);

    bool                         stop_requested   = engine->stop_requested;
    bal_engine_t *POUND_RESTRICT ballistic_engine = &engine->ballistic_engine;
    bal_cpu_t *POUND_RESTRICT    ballistic_cpu    = &engine->cpu;
    jit_stats_t *POUND_RESTRICT  jit_stats        = &engine->stats;

    while (false == stop_requested)
    {
        // TODO: implement threading API.
    }
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

POUND_COLD error_t
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