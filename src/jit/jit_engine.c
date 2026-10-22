#include "jit_engine.h"

#include "log.h"

#include <string.h>

static const uint8_t *ballistic_translate_guest_address(void               *context,
                                                        bal_guest_address_t guest_address,
                                                        size_t             *max_readable);

error_t
jit_engine_init(jit_engine_t *context, guest_memory_t *guest_memory, const uint32_t flags)
{
    if (POUND_UNLIKELY(NULL == context))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: context is NULL.");
        context->status = POUND_ERROR_INVALID_ARGUMENT;
        return context->status;
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

    (void)memset(context, 0, sizeof(jit_engine_t));
    context->guest_memory   = guest_memory;
    context->engine_flags   = flags;
    context->stop_requested = false;

    memset(&context->cpu, 0, sizeof(bal_cpu_t));
    context->ballistic_memory_interface.context   = guest_memory;
    context->ballistic_memory_interface.translate = ballistic_translate_guest_address;

    POUND_LOG_DEBUG(&thread_logger,
                    "Memory interface bridge configured. "
                    "guest range [0x%llx, 0x%llx), host_base=%p.",
                    (unsigned long long)guest_memory->guest_base,
                    (unsigned long long)guest_memory->guest_end,
                    (void *)guest_memory->host_base);

    bal_allocator_default_init(&context->allocator);
    POUND_LOG_DEBUG(&thread_logger, "Default allocator initialized.");

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
    POUND_LOG_INFO(&thread_logger, "Ballistic engine ready.");
    return context->status;
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