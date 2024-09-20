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

VkImageView CreateImageView(VulkanContext &ctx, VkImage image, VkFormat format,
                            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

VkCommandBuffer BeginSingleTimeCommands(VulkanContext &ctx, VkCommandPool commandPool);
void EndSingleTimeCommands(VulkanContext &ctx, VkQueue queue, VkCommandPool commandPool,
                           VkCommandBuffer commandBuffer);

struct CopyBufferInfo {
    VkQueue Queue;
    VkCommandPool Pool;
    VkBuffer Src;
    VkBuffer Dst;
    VkDeviceSize Size;
};

void CopyBuffer(VulkanContext &ctx, CopyBufferInfo info);

struct TransitionImageLayoutInfo {
    VkQueue Queue;
    VkCommandPool Pool;
    VkImage Image;
    VkFormat Format;
    VkImageLayout OldLayout;
    VkImageLayout NewLayout;
};

void TransitionImageLayout(VulkanContext &ctx, TransitionImageLayoutInfo info);

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

struct CopyBufferToImageInfo {
    VkQueue Queue;
    VkCommandPool Pool;
    VkBuffer Buffer;
    VkImage Image;
    uint32_t Width;
    uint32_t Height;
};

void CopyBufferToImage(VulkanContext &ctx, CopyBufferToImageInfo info);

VkFormat FindSupportedFormat(VulkanContext &ctx, const std::vector<VkFormat> &candidates,
                             VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat FindDepthFormat(VulkanContext &ctx);

bool HasStencilComponent(VkFormat format);

} // namespace utils