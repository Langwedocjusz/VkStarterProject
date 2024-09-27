#include "Common.h"

#include "Utils.h"

void common::ViewportScissorDefaultBehaviour(VulkanContext &ctx, VkCommandBuffer buffer)
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(ctx.Swapchain.extent.width);
    viewport.height = static_cast<float>(ctx.Swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = ctx.Swapchain.extent;
    vkCmdSetScissor(buffer, 0, 1, &scissor);
}

void common::ImageBarrierColorToRender(VkCommandBuffer buffer, VkImage swapchainImage)
{
    utils::ImageMemoryBarrierInfo info{
        swapchainImage,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    utils::InsertImageMemoryBarrier(buffer, info);
}

void common::ImageBarrierColorToPresent(VkCommandBuffer buffer, VkImage swapchainImage)
{
    utils::ImageMemoryBarrierInfo info{
        swapchainImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    utils::InsertImageMemoryBarrier(buffer, info);
}

void common::ImageBarrierDepthToRender(VkCommandBuffer buffer, VkImage depthImage)
{
    utils::ImageMemoryBarrierInfo info{
        depthImage,
        0,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}};

    utils::InsertImageMemoryBarrier(buffer, info);
}