#pragma once

#include "VulkanContext.h"

namespace common
{
void ViewportScissorDefaultBehaviour(VulkanContext &ctx, VkCommandBuffer buffer);

void ImageBarrierColorToRender(VkCommandBuffer buffer, VkImage swapchainImage);
void ImageBarrierColorToPresent(VkCommandBuffer buffer, VkImage swapchainImage);

void ImageBarrierDepthToRender(VkCommandBuffer buffer, VkImage depthImage);
} // namespace common