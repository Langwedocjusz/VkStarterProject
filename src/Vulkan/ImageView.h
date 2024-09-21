#pragma once

#include "VulkanContext.h"

namespace ImageView
{
VkImageView Create(VulkanContext &ctx, VkImage image, VkFormat format,
                   VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
}
