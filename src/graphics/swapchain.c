#include "swapchain.h"
#include "log.h"

#define GRAPHICS_MAX_SURFACE_FORMATS 128
#define GRAPHICS_MAX_PRESENT_MODES   8

POUND_HOT static pound_error_t surface_format_select(graphics_state_t *POUND_RESTRICT state);
POUND_HOT static pound_error_t present_mode_select(graphics_state_t *POUND_RESTRICT state);
POUND_HOT static pound_error_t image_views_create(graphics_state_t *POUND_RESTRICT state);
POUND_HOT static pound_error_t render_pass_create(graphics_state_t *POUND_RESTRICT state);

POUND_HOT pound_error_t
swapchain_create(graphics_state_t *POUND_RESTRICT state)
{
    if (NULL == state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state is NULL.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == state->device)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: device is NULl, cannot create swapchain.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == state->surface)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: surface is NULl, cannot create swapchain.");
        return POUND_ERROR_INVALID_ARGUMENT;
    }

    VkSurfaceCapabilitiesKHR capabilities = { 0 };
    VkResult                 result       = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        state->physical_device, state->surface, &capabilities);

    if (POUND_UNLIKELY(VK_SUCCESS != result))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetPhysicalDeviceSurfaceCapabilitiesKHR() returned "
                        "%d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    VkExtent2D extent = { 0 };

    if (UINT32_MAX != capabilities.currentExtent.width)
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        extent.width  = state->drawable_width;
        extent.height = state->drawable_height;

        if (extent.width < capabilities.minImageExtent.width)
        {
            extent.width = capabilities.minImageExtent.width;
        }
        if (extent.width > capabilities.maxImageExtent.width)
        {
            extent.width = capabilities.maxImageExtent.width;
        }
        if (extent.height < capabilities.minImageExtent.height)
        {
            extent.height = capabilities.minImageExtent.height;
        }
        if (extent.height > capabilities.maxImageExtent.height)
        {
            extent.height = capabilities.maxImageExtent.height;
        }
    }

    if (POUND_UNLIKELY(0 == extent.width || 0 == extent.height))
    {
        POUND_LOG_WARN(&thread_logger,
                       "Skipping swapchain creation: surface extent is zero (window minimized).");
        state->state_flags |= GRAPHICS_STATE_FLAG_MINIMIZED;
        return state->status;
    }

    state->state_flags &= (uint32_t)~GRAPHICS_STATE_FLAG_MINIMIZED;
    pound_error_t error = surface_format_select(state);

    if (error != POUND_SUCCESS)
    {
        return error;
    }

    error = present_mode_select(state);

    if (error != POUND_SUCCESS)
    {
        return error;
    }

    uint32_t image_count = capabilities.minImageCount + 1U;

    if (capabilities.maxImageCount != 0 && image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    if (image_count > GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY)
    {
        POUND_LOG_WARN(&thread_logger,
                       "Surface requested %u images, clamping to %d.",
                       image_count,
                       GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY);
        image_count = GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY;
    }

    if (image_count < 2U)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: surface supports only %u image(s), need at least 2.",
                        image_count);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    const uint32_t families[2]
        = { state->capabilities.graphics_queue_family, state->capabilities.present_queue_family };
    const bool concurrent = (families[0] != families[1]);

    VkSwapchainCreateInfoKHR create_info = { 0 };
    create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface                  = state->surface;
    create_info.minImageCount            = image_count;
    create_info.imageFormat              = state->surface_format.format;
    create_info.imageColorSpace          = state->surface_format.colorSpace;
    create_info.imageExtent              = extent;
    create_info.imageArrayLayers         = 1;
    create_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode
        = concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = concurrent ? 2U : 0U;
    create_info.pQueueFamilyIndices   = concurrent ? families : NULL;
    create_info.preTransform          = capabilities.currentTransform;
    create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode           = state->present_mode;
    create_info.clipped               = VK_TRUE;
    create_info.oldSwapchain          = state->swapchain;

    VkSwapchainKHR new_swapchain = NULL;
    result = vkCreateSwapchainKHR(state->device, &create_info, NULL, &new_swapchain);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkCreateSwapchainKHR() returned %d.", (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (state->swapchain != NULL)
    {
        vkDestroySwapchainKHR(state->device, state->swapchain, NULL);
    }

    state->swapchain        = new_swapchain;
    state->swapchain_extent = extent;
    uint32_t actual_count   = 0U;
    result = vkGetSwapchainImagesKHR(state->device, state->swapchain, &actual_count, NULL);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetSwapchainImagesKHR() returned %d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (actual_count > GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: swapchain returned %u images, capacity is %d.",
                        actual_count,
                        GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    VkImage images[GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY];
    result = vkGetSwapchainImagesKHR(state->device, state->swapchain, &actual_count, images);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetSwapchainImagesKHR() returned %d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    state->swapchain_image_count              = actual_count;
    state->capabilities.swapchain_image_count = actual_count;

    graphics_swapchain_image_t *POUND_RESTRICT swapchain_image_cursor = state->swapchain_images;
    const VkImage *POUND_RESTRICT              image_cursor           = images;

    for (uint32_t i = 0; i < actual_count; ++i)
    {
        swapchain_image_cursor->image = *image_cursor++;
        ++swapchain_image_cursor;
    }

    error = image_views_create(state);

    if (error != POUND_SUCCESS)
    {
        return error;
    }

    error = render_pass_create(state);

    if (error != POUND_SUCCESS)
    {
        return error;
    }

    VkSemaphoreCreateInfo semaphore_info = { 0 };
    semaphore_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    swapchain_image_cursor = state->swapchain_images;

    for (uint32_t i = 0U; i < actual_count; ++i)
    {
        VkFramebufferCreateInfo framebuffer_info = { 0 };
        framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass              = state->render_pass;
        framebuffer_info.attachmentCount         = 1;
        framebuffer_info.pAttachments            = &swapchain_image_cursor->image_view;
        framebuffer_info.width                   = extent.width;
        framebuffer_info.height                  = extent.height;
        framebuffer_info.layers                  = 1;

        result = vkCreateFramebuffer(
            state->device, &framebuffer_info, NULL, &swapchain_image_cursor->framebuffer);

        if (result != VK_SUCCESS)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: vkCreateFramebuffer() returned %d for image %u.",
                            (int)result,
                            i);
            swapchain_image_cursor->framebuffer = NULL;
            state->status                       = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }

        result = vkCreateSemaphore(state->device,
                                   &semaphore_info,
                                   NULL,
                                   &swapchain_image_cursor->render_finished_semaphore);

        if (result != VK_SUCCESS)
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: vkCreateSemaphore() returned %d for image %u.",
                            (int)result,
                            i);
            swapchain_image_cursor->render_finished_semaphore = NULL;
            state->status                                     = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }

        ++swapchain_image_cursor;
    }

    state->state_flags &= (uint32_t)~GRAPHICS_STATE_FLAG_SWAPCHAIN_DIRTY;
    return state->status;
}

POUND_HOT void
swapchain_destroy(graphics_state_t *POUND_RESTRICT state)
{
    if (NULL == state)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state is NULL.");
        return;
    }

    if (NULL == state->device)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->device is NULL.");
        return;
    }

    graphics_swapchain_image_t *POUND_RESTRICT swapchain_image_cursor = state->swapchain_images;

    for (uint32_t i = 0; i < GRAPHICS_SWAPCHAIN_IMAGE_CAPACITY; ++i)
    {
        if (NULL != swapchain_image_cursor->render_finished_semaphore)
        {
            vkDestroySemaphore(
                state->device, swapchain_image_cursor->render_finished_semaphore, NULL);
            swapchain_image_cursor->render_finished_semaphore = NULL;
        }
        if (NULL != swapchain_image_cursor->framebuffer)
        {
            vkDestroyFramebuffer(state->device, swapchain_image_cursor->framebuffer, NULL);
            swapchain_image_cursor->framebuffer = NULL;
        }
        if (NULL != swapchain_image_cursor->image_view)
        {
            vkDestroyImageView(state->device, swapchain_image_cursor->image_view, NULL);
            swapchain_image_cursor->image_view = NULL;
        }

        // The VkImage is owned by the swapchain, not by us.
        swapchain_image_cursor->image = NULL;
        ++swapchain_image_cursor;
    }

    if (state->render_pass != NULL)
    {
        vkDestroyRenderPass(state->device, state->render_pass, NULL);
        state->render_pass = NULL;
    }

    state->swapchain_image_count = 0;
}

POUND_HOT pound_error_t
graphics_subsystem_begin_frame(void)
{
    graphics_state_t *POUND_RESTRICT state = &g_graphics_state;

    if (POUND_UNLIKELY(false == state->initialized))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: graphics subsystem is not initialized.");

        state->status = POUND_ERROR_GRAPHICS_NOT_INITIALIZED;
        return state->status;
    }

    if (POUND_UNLIKELY(state->status != POUND_SUCCESS))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->status != POUND_SUCCESS.");
        return state->status;
    }

    if (POUND_UNLIKELY((state->state_flags & GRAPHICS_STATE_FLAG_FRAME_ACTIVE) != 0))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: a frame is already active, call end_frame first.");
        state->status = POUND_ERROR_GRAPHICS_START_FRAME_FAILED;
        return state->status;
    }

    if ((state->state_flags & GRAPHICS_STATE_FLAG_SWAPCHAIN_DIRTY) != 0)
    {
        const pound_error_t error = graphics_subsystem_recreate_swapchain();

        if (POUND_UNLIKELY(error != POUND_SUCCESS))
        {
            return error;
        }
    }

    if (POUND_UNLIKELY((state->state_flags & GRAPHICS_STATE_FLAG_MINIMIZED) != 0))
    {
        return POUND_ERROR_GRAPHICS_ZERO_EXTENT;
    }

    if (POUND_UNLIKELY(NULL == state->swapchain))
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: state->swapchain is NULL.");
        return POUND_ERROR_GRAPHICS_NOT_INITIALIZED;
    }

    graphics_frame_t *POUND_RESTRICT frame = &state->frames[state->current_frame_slot];
    VkResult                         result
        = vkWaitForFences(state->device, 1, &frame->in_flight_fence, VK_TRUE, UINT64_MAX);

    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkWaitForFences() returned %d on frame slot %u.",
                        (int)result,
                        state->current_frame_slot);
        state->status = VK_ERROR_DEVICE_LOST == result ? POUND_ERROR_GRAPHICS_DEVICE_LOST
                                                       : POUND_ERROR_GRAPHICS_FENCE_WAIT_FAILED;
        return state->status;
    }

    uint32_t image_index = 0U;
    result               = vkAcquireNextImageKHR(state->device,
                                                 state->swapchain,
                                                 UINT64_MAX,
                                                 frame->image_available_semaphore,
                                                 NULL,
                                                 &image_index);

    if (VK_ERROR_OUT_OF_DATE_KHR == result)
    {
        state->state_flags |= GRAPHICS_STATE_FLAG_SWAPCHAIN_DIRTY;
        POUND_LOG_DEBUG(&thread_logger, "Swapchain out of date at acquire, skipping frame.");
        return POUND_ERROR_GRAPHICS_SWAPCHAIN_OUT_OF_DATE;
    }

    if (VK_SUBOPTIMAL_KHR == result)
    {
        state->state_flags |= GRAPHICS_STATE_FLAG_SWAPCHAIN_DIRTY;
        POUND_LOG_DEBUG(&thread_logger, "Swapchain is suboptimal at acquire, rendering anyway.");
    }
    else if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkAcquireNextImageKHR() returned %d.", (int)result);
        state->status = VK_ERROR_DEVICE_LOST == result ? POUND_ERROR_GRAPHICS_DEVICE_LOST
                                                       : POUND_ERROR_GRAPHICS_START_FRAME_FAILED;
        return state->status;
    }

    if (POUND_UNLIKELY(image_index >= state->swapchain_image_count))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: acquired image index %u exceeds image count %u.",
                        image_index,
                        state->swapchain_image_count);
        state->status = POUND_ERROR_GRAPHICS_IMAGE_ACQUIRE_FAILED;
        return state->status;
    }

    // The fence is only reset after a successful acquire. If acquire fails the
    // fence stays signaled, so the next begin_frame wait cannot deadlock.
    result = vkResetFences(state->device, 1U, &frame->in_flight_fence);

    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkResetFences returned %d.", (int)result);
        state->status = POUND_ERROR_GRAPHICS_FENCE_RESET_FAILED;
        return state->status;
    }

    result = vkResetCommandBuffer(frame->command_buffer, 0U);

    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkResetCommandBuffer returned %d.", (int)result);
        state->status = POUND_ERROR_GRAPHICS_COMMAND_BUFFER_RESET_FAILED;
        return state->status;
    }

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(frame->command_buffer, &begin_info);

    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkBeginCommandBuffer returned %d.", (int)result);
        state->status = POUND_ERROR_GRAPHICS_COMMAND_BUFFER_BEGIN_FAILED;
        return state->status;
    }

    VkClearValue clear     = { 0 };
    clear.color.float32[0] = 0.06F;
    clear.color.float32[1] = 0.06F;
    clear.color.float32[2] = 0.06F;
    clear.color.float32[3] = 1.0F;

    VkRenderPassBeginInfo pass_info = { 0 };
    pass_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_info.renderPass            = state->render_pass;
    pass_info.framebuffer           = state->swapchain_images[image_index].framebuffer;
    pass_info.renderArea.offset     = (VkOffset2D) { .x = 0, .y = 0 };
    pass_info.renderArea.extent     = state->swapchain_extent;
    pass_info.clearValueCount       = 1;
    pass_info.pClearValues          = &clear;

    vkCmdBeginRenderPass(frame->command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.x        = 0.0F;
    viewport.y        = 0.0F;
    viewport.width    = (float)state->swapchain_extent.width;
    viewport.height   = (float)state->swapchain_extent.height;
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(frame->command_buffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = (VkOffset2D) { .x = 0, .y = 0 };
    scissor.extent = state->swapchain_extent;
    vkCmdSetScissor(frame->command_buffer, 0, 1, &scissor);

    frame->image_index = image_index;
    state->state_flags |= GRAPHICS_STATE_FLAG_FRAME_ACTIVE;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();
    return state->status;
}

POUND_HOT pound_error_t
graphics_subsystem_end_frame(void)
{
    graphics_state_t *POUND_RESTRICT state = &g_graphics_state;

    if (POUND_UNLIKELY(false == state->initialized))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: graphics subsystem is not initialized.");
        return POUND_ERROR_GRAPHICS_NOT_INITIALIZED;
    }
    if (POUND_UNLIKELY(0 == (state->state_flags & GRAPHICS_STATE_FLAG_FRAME_ACTIVE)))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: no frame is active, call begin_frame first.");
        state->status = POUND_ERROR_GRAPHICS_END_FRAME_FAILED;
        return state->status;
    }

    graphics_frame_t *POUND_RESTRICT frame = &state->frames[state->current_frame_slot];
    const graphics_swapchain_image_t *POUND_RESTRICT image
        = &state->swapchain_images[frame->image_index];

    igRender();
    ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), frame->command_buffer, NULL);
    vkCmdEndRenderPass(frame->command_buffer);
    VkResult result = vkEndCommandBuffer(frame->command_buffer);

    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkEndCommandBuffer returned %d.", (int)result);
        state->state_flags &= (uint32_t)~GRAPHICS_STATE_FLAG_FRAME_ACTIVE;
        state->status = POUND_ERROR_GRAPHICS_COMMAND_BUFFER_END_FAILED;
        return state->status;
    }

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info         = { 0 };
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &frame->image_available_semaphore;
    submit_info.pWaitDstStageMask    = &wait_stage;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &frame->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &image->render_finished_semaphore;

    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, frame->in_flight_fence);

    if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkQueueSubmit returned %d.", (int)result);
        state->state_flags &= (uint32_t)~GRAPHICS_STATE_FLAG_FRAME_ACTIVE;
        state->status = (VK_ERROR_DEVICE_LOST == result) ? POUND_ERROR_GRAPHICS_DEVICE_LOST
                                                         : POUND_ERROR_GRAPHICS_QUEUE_SUBMIT_FAILED;
        return state->status;
    }

    ++state->submit_count;
    ++frame->submission_index;

    VkPresentInfoKHR present_info   = { 0 };
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &image->render_finished_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &state->swapchain;
    present_info.pImageIndices      = &frame->image_index;

    result = vkQueuePresentKHR(state->present_queue, &present_info);

    if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result)
    {
        state->state_flags |= GRAPHICS_STATE_FLAG_SWAPCHAIN_DIRTY;
        POUND_LOG_DEBUG(
            &thread_logger, "Swapchain needs recreation after present (%d).", (int)result);
    }
    else if (POUND_UNLIKELY(result != VK_SUCCESS))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkQueuePresentKHR returned %d.", (int)result);
        state->state_flags &= (uint32_t)~GRAPHICS_STATE_FLAG_FRAME_ACTIVE;
        state->status = (VK_ERROR_DEVICE_LOST == result)
                            ? POUND_ERROR_GRAPHICS_DEVICE_LOST
                            : POUND_ERROR_GRAPHICS_QUEUE_PRESENT_FAILED;
        return state->status;
    }
    else
    {
        ++state->present_count;
    }

    state->state_flags &= (uint32_t)~GRAPHICS_STATE_FLAG_FRAME_ACTIVE;
    state->current_frame_slot = (state->current_frame_slot + 1U) % GRAPHICS_FRAMES_IN_FLIGHT;
    ++state->frame_index;
    return state->status;
}

POUND_HOT pound_error_t
graphics_subsystem_recreate_swapchain(void)
{
    graphics_state_t *POUND_RESTRICT state = &g_graphics_state;

    if (POUND_UNLIKELY(false == state->initialized))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: graphics subsystem is not initialized.");
        state->status = POUND_ERROR_GRAPHICS_NOT_INITIALIZED;
        return state->status;
    }

    if (POUND_UNLIKELY((state->state_flags & GRAPHICS_STATE_FLAG_FRAME_ACTIVE) != 0))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: cannot recreate the swapchain mid-frame.");
        state->status = POUND_ERROR_GRAPHICS_START_FRAME_FAILED;
        return state->status;
    }

    int width  = 0;
    int height = 0;

    if (false == SDL_GetWindowSizeInPixels(state->window, &width, &height))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: SDL_Vulkan_GetDrawableSize failed because %s.",
                        SDL_GetError());
        state->status = POUND_ERROR_GRAPHICS_START_FRAME_FAILED;
        return state->status;
    }

    if (width <= 0 || height <= 0)
    {
        state->state_flags |= GRAPHICS_STATE_FLAG_MINIMIZED;
        POUND_LOG_DEBUG(&thread_logger, "Window minimized, deferring swapchain recreation.");
        return POUND_ERROR_GRAPHICS_ZERO_EXTENT;
        ;
    }

    state->drawable_width  = (uint32_t)width;
    state->drawable_height = (uint32_t)height;

    const VkResult idle = vkDeviceWaitIdle(state->device);

    if (POUND_UNLIKELY(idle != VK_SUCCESS))
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkDeviceWaitIdle() returned %d before recreation.",
                        (int)idle);
        state->status = VK_ERROR_DEVICE_LOST == idle ? POUND_ERROR_GRAPHICS_DEVICE_LOST
                                                     : POUND_ERROR_GRAPHICS_FENCE_WAIT_FAILED;
        return state->status;
    }

    swapchain_destroy(state);

    const pound_error_t error = swapchain_create(state);

    if (POUND_UNLIKELY(error != POUND_SUCCESS))
    {
        return error;
    }

    ImGui_ImplVulkan_SetMinImageCount(state->swapchain_image_count);

    POUND_LOG_DEBUG(&thread_logger,
                    "Swapchain recreated at %ux%u with %u images.",
                    state->swapchain_extent.width,
                    state->swapchain_extent.height,
                    state->swapchain_image_count);
    return state->status;
}

POUND_HOT static pound_error_t
surface_format_select(graphics_state_t *POUND_RESTRICT state)
{
    uint32_t count  = 0U;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        state->physical_device, state->surface, &count, NULL);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetPhysicalDeviceSurfaceFormatsKHR() returned %d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (0 == count)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: the surface reports no usable color formats.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    VkSurfaceFormatKHR formats[GRAPHICS_MAX_SURFACE_FORMATS];

    if (count > GRAPHICS_MAX_SURFACE_FORMATS)
    {
        count = GRAPHICS_MAX_SURFACE_FORMATS;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        state->physical_device, state->surface, &count, formats);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetPhysicalDeviceSurfaceFormatsKHR() returned %d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    state->surface_format                                  = formats[0];
    const VkSurfaceFormatKHR *POUND_RESTRICT format_cursor = formats;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (VK_FORMAT_B8G8R8A8_UNORM == format_cursor->format
            && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == format_cursor->colorSpace)
        {
            state->surface_format = *format_cursor;
            break;
        }

        ++format_cursor;
    }

    return state->status;
}

pound_error_t
present_mode_select(graphics_state_t *POUND_RESTRICT state)
{
    uint32_t count  = 0U;
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        state->physical_device, state->surface, &count, NULL);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetPhysicalDeviceSurfacePresentModesKHR() returned "
                        "%d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    if (0 == count)
    {
        POUND_LOG_ERROR(&thread_logger, "Aborting function: the surface reports no present modes.");
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    VkPresentModeKHR modes[GRAPHICS_MAX_PRESENT_MODES];

    if (count > GRAPHICS_MAX_PRESENT_MODES)
    {
        count = GRAPHICS_MAX_PRESENT_MODES;
    }

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        state->physical_device, state->surface, &count, modes);

    if (result != VK_SUCCESS)
    {
        POUND_LOG_ERROR(&thread_logger,
                        "Aborting function: vkGetPhysicalDeviceSurfacePresentModesKHR() returned "
                        "%d.",
                        (int)result);
        state->status = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    state->present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if ((state->state_flags & GRAPHICS_STATE_FLAG_VSYNC) != 0)
    {
        return state->status;
    }

    bool                                   has_mailbox = false;
    const VkPresentModeKHR *POUND_RESTRICT mode_cursor = modes;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (VK_PRESENT_MODE_IMMEDIATE_KHR == *mode_cursor)
        {
            state->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            return state->status;
        }
        if (VK_PRESENT_MODE_MAILBOX_KHR == *mode_cursor)
        {
            has_mailbox = true;
        }
        ++mode_cursor;
    }
    if (has_mailbox)
    {
        state->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return state->status;
}

POUND_HOT static pound_error_t
image_views_create(graphics_state_t *POUND_RESTRICT state)
{
    graphics_swapchain_image_t *POUND_RESTRICT cursor = state->swapchain_images;

    for (uint32_t i = 0; i < state->swapchain_image_count; ++i)
    {
        VkImageViewCreateInfo create_info           = { 0 };
        create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image                           = cursor->image;
        create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format                          = state->surface_format.format;
        create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel   = 0;
        create_info.subresourceRange.levelCount     = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount     = 1;

        const VkResult result
            = vkCreateImageView(state->device, &create_info, NULL, &cursor->image_view);

        if (POUND_UNLIKELY(VK_SUCCESS != result))
        {
            POUND_LOG_ERROR(&thread_logger,
                            "Aborting function: vkCreateImageView returned %d for image %u.",
                            (int)result,
                            i);
            cursor->image_view = NULL;
            state->status      = POUND_ERROR_GRAPHICS_INIT_FAILED;
            return state->status;
        }

        ++cursor;
    }

    return state->status;
}

POUND_HOT static pound_error_t
render_pass_create(graphics_state_t *POUND_RESTRICT state)
{
    VkAttachmentDescription attachment = { 0 };
    attachment.format                  = state->surface_format.format;
    attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference reference;
    reference.attachment = 0;
    reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &reference;

    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = 0;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo create_info = { 0 };
    create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount        = 1;
    create_info.pAttachments           = &attachment;
    create_info.subpassCount           = 1;
    create_info.pSubpasses             = &subpass;
    create_info.dependencyCount        = 1;
    create_info.pDependencies          = &dependency;

    const VkResult result
        = vkCreateRenderPass(state->device, &create_info, NULL, &state->render_pass);

    if (POUND_UNLIKELY(VK_SUCCESS != result))
    {
        POUND_LOG_ERROR(
            &thread_logger, "Aborting function: vkCreateRenderPass() returned %d.", (int)result);
        state->render_pass = NULL;
        state->status      = POUND_ERROR_GRAPHICS_INIT_FAILED;
        return state->status;
    }

    return state->status;
}