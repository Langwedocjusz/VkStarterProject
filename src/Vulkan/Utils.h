#pragma once

#include <string>
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

void ViewportScissorDefaultBehaviour(VulkanContext &ctx, VkCommandBuffer buffer);

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

void ImageBarrierColorToRender(VkCommandBuffer buffer, VkImage swapchainImage);
void ImageBarrierColorToPresent(VkCommandBuffer buffer, VkImage swapchainImage);

void ImageBarrierDepthToRender(VkCommandBuffer buffer, VkImage depthImage);
} // namespace utils