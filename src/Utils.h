#pragma once

#include <string>
#include <vector>

#include "VulkanContext.h"

namespace utils
{
std::vector<char> ReadFileBinary(const std::string &filename);

VkShaderModule CreateShaderModule(VulkanContext &ctx, const std::vector<char> &code);

uint32_t FindMemoryType(VulkanContext &ctx, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);

void CreateBuffer(VulkanContext &ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);

struct CreateImageInfo {
    uint32_t Width;
    uint32_t Height;
    VkFormat Format;
    VkImageTiling Tiling;
    VkImageUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

void CreateImage(VulkanContext &ctx, VkImage &image, VkDeviceMemory &imageMemory,
                 CreateImageInfo info);

VkImageView CreateImageView(VulkanContext &ctx, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

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

struct CopyBufferToImageInfo {
    VkQueue Queue;
    VkCommandPool Pool;
    VkBuffer Buffer;
    VkImage Image;
    uint32_t Width;
    uint32_t Height;
};

void CopyBufferToImage(VulkanContext &ctx, CopyBufferToImageInfo info);

VkFormat FindSupportedFormat(VulkanContext &ctx, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat FindDepthFormat(VulkanContext &ctx);

bool HasStencilComponent(VkFormat format);

} // namespace utils