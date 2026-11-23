#ifndef POUND_SWAPCHAIN_H
#define POUND_SWAPCHAIN_H

#include "graphics.h"

POUND_HOT pound_error_t swapchain_create(graphics_state_t *POUND_RESTRICT state);
POUND_HOT void          swapchain_destroy(graphics_state_t *POUND_RESTRICT state);

#endif // POUND_SWAPCHAIN_H
