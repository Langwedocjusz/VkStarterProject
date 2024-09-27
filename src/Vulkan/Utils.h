#pragma once

#include <vector>

#include "VulkanContext.h"

namespace utils
{
template <typename VertexType>
VkVertexInputBindingDescription GetBindingDescription(uint32_t binding,
                                                      VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = binding;
    bindingDescription.stride = sizeof(VertexType);
    bindingDescription.inputRate = inputRate;
    return bindingDescription;
}

VkCommandBuffer BeginSingleTimeCommands(VulkanContext &ctx, VkCommandPool commandPool);

void EndSingleTimeCommands(VulkanContext &ctx, VkQueue queue, VkCommandPool commandPool,
                           VkCommandBuffer commandBuffer);

VkFormat FindSupportedFormat(VulkanContext &ctx, const std::vector<VkFormat> &candidates,
                             VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat FindDepthFormat(VulkanContext &ctx);

bool HasStencilComponent(VkFormat format);

struct ImageMemoryBarrierInfo {
    VkImage Image;
    VkAccessFlags SrcAccessMask;
    VkAccessFlags DstAccessMask;
    VkImageLayout OldImageLayout;
    VkImageLayout NewImageLayout;
    VkPipelineStageFlags SrcStageMask;
    VkPipelineStageFlags DstStageMask;
    VkImageSubresourceRange SubresourceRange;
};

void InsertImageMemoryBarrier(VkCommandBuffer buffer, ImageMemoryBarrierInfo info);
} // namespace utils