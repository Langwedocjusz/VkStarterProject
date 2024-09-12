#pragma once

#include "RendererBase.h"

class HelloTriangleRenderer : public RendererBase {
  public:
    void OnImGui() override;

  private:
    VkRenderPass getImGuiRenderPass() const override
    {
        return RenderPass;
    }

    void CreatePermanentResources(VulkanContext &ctx) override;
    void CreateSwapchainResources(VulkanContext &ctx) override;

    void DestroyPermanentResources(VulkanContext &ctx) override;
    void DestroySwapchainResources(VulkanContext &ctx) override;

    void SubmitCommandBuffers(VulkanContext &ctx) override;

  private:
    void CreateRenderPasses(VulkanContext &ctx);
    void CreateGraphicsPipelines(VulkanContext &ctx);

    void CreateCommandPools(VulkanContext &ctx);
    void CreateCommandBuffers(VulkanContext &ctx);
    void CreateFramebuffers(VulkanContext &ctx);

    void RecordCommandBuffer(VulkanContext &ctx, VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

  private:
    VkRenderPass RenderPass;

    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    bool show_demo_window = true;
};