#include "Utils.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkQueue utils::GetQueue(VulkanContext &ctx, vkb::QueueType type)
{
    auto queue = ctx.Device.get_queue(type);

    if (!queue.has_value())
    {
        auto err_msg = "Failed to get a queue: " + queue.error().message();
        throw std::runtime_error(err_msg);
    }

    return queue.value();
}

void utils::CreateSignalledFence(VulkanContext &ctx, VkFence &fence)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(ctx.Device, &fence_info, nullptr, &fence) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a fence!");
}

void utils::CreateSemaphore(VulkanContext &ctx, VkSemaphore &semaphore)
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(ctx.Device, &semaphore_info, nullptr, &semaphore) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a semaphore!");
}

utils::ScopedCommand::ScopedCommand(VulkanContext &ctx, VkQueue queue, VkCommandPool commandPool)
    : ctx(ctx), mQueue(queue), mCommandPool(commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(ctx.Device, &allocInfo, &Buffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(Buffer, &beginInfo);
}

utils::ScopedCommand::~ScopedCommand()
{
    vkEndCommandBuffer(Buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &Buffer;

    vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mQueue);

    vkFreeCommandBuffers(ctx.Device, mCommandPool, 1, &Buffer);
}

void utils::InsertImageMemoryBarrier(VkCommandBuffer buffer, ImageMemoryBarrierInfo info)
{
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    imageMemoryBarrier.srcAccessMask = info.SrcAccessMask;
    imageMemoryBarrier.dstAccessMask = info.DstAccessMask;
    imageMemoryBarrier.oldLayout = info.OldImageLayout;
    imageMemoryBarrier.newLayout = info.NewImageLayout;
    imageMemoryBarrier.image = info.Image;
    imageMemoryBarrier.subresourceRange = info.SubresourceRange;

    vkCmdPipelineBarrier(buffer, info.SrcStageMask, info.DstStageMask, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
}

VkFormat utils::FindSupportedFormat(VulkanContext &ctx,
                                    const std::vector<VkFormat> &candidates,
                                    VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx.PhysicalDevice, format, &props);

        bool req_linear = (tiling == VK_IMAGE_TILING_LINEAR);
        bool req_optimal = (tiling == VK_IMAGE_TILING_OPTIMAL);

        bool has_linear = (props.linearTilingFeatures & features) == features;
        bool has_optimal = (props.optimalTilingFeatures & features) == features;

        bool linear = req_linear && has_linear;
        bool optimal = req_optimal && has_optimal;

        if (linear || optimal)
            return format;
    }

    throw std::runtime_error("Failed to find supported format!");
}

VkFormat utils::FindDepthFormat(VulkanContext &ctx)
{
    std::vector<VkFormat> candidates{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                     VK_FORMAT_D24_UNORM_S8_UINT};

    return FindSupportedFormat(ctx, candidates, VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool utils::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}