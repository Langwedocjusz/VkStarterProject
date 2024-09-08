#pragma once

#include "VulkanContext.h"

class Renderer {
  public:
    void OnInit(VulkanContext &ctx);
    void OnUpdate();
    void OnImGui();
    void OnRender(VulkanContext &ctx);

    void RecreateSwapchain(VulkanContext &ctx);

    void VulkanCleanup(VulkanContext &ctx);

    // Temporary way of syncing things with imgui:
    VkQueue getGraphicsQueue() const
    {
        return GraphicsQueue;
    }
    VkRenderPass getRenderPass() const
    {
        return RenderPass;
    }
    int getFramesInFlight() const
    {
        return MAX_FRAMES_IN_FLIGHT;
    }
    VkSampleCountFlagBits getMSAACount() const
    {
        return VK_SAMPLE_COUNT_1_BIT;
    }

  private:
    void CreateQueues(VulkanContext &ctx);
    void CreateRenderPass(VulkanContext &ctx);
    void CreateGraphicsPipeline(VulkanContext &ctx);
    void CreateSyncObjects(VulkanContext &ctx);

    void CreateFramebuffers(VulkanContext &ctx);
    void CreateCommandPool(VulkanContext &ctx);
    void CreateCommandBuffers(VulkanContext &ctx);

    void RecordCommandBuffer(VulkanContext &ctx, VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void DrawFrame(VulkanContext &ctx);
    void PresentFrame(VulkanContext &ctx);

  private:
    const size_t MAX_FRAMES_IN_FLIGHT = 2;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;

    std::vector<VkImage> SwapchainImages;
    std::vector<VkImageView> SwapchainImageViews;
    std::vector<VkFramebuffer> Framebuffers;

    VkRenderPass RenderPass;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    std::vector<VkSemaphore> ImageAcquiredSemaphores;
    std::vector<VkSemaphore> RenderCompletedSemaphores;

    std::vector<VkFence> InFlightFences;

    size_t FrameSemaphoreIndex = 0;
    uint32_t FrameImageIndex = 0;

    bool show_demo_window = true;
};