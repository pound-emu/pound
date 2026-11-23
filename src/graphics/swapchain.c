#include "swapchain.h"
#include "log.h"

#define GRAPHICS_MAX_SURFACE_FORMATS 32
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

    if (result == VK_SUCCESS)
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