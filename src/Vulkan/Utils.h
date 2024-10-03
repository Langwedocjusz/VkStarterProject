#pragma once

#include <vector>

#include "VkBootstrap.h"
#include "VulkanContext.h"

namespace utils
{
VkQueue GetQueue(VulkanContext &ctx, vkb::QueueType type);

void CreateSignalledFence(VulkanContext &ctx, VkFence &fence);
void CreateSemaphore(VulkanContext &ctx, VkSemaphore &semaphore);

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

class ScopedCommand{
public:
    ScopedCommand(VulkanContext &ctx, VkQueue queue, VkCommandPool commandPool);
    ~ScopedCommand();
public:
    VkCommandBuffer Buffer;
private:
    VulkanContext &ctx;
    VkQueue mQueue;
    VkCommandPool mCommandPool;
};

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