#pragma once

#include "RendererBase.h"

#include <glm/glm.hpp>

class MainMenuRenderer : public RendererBase {
  public:
    MainMenuRenderer(VulkanContext& ctx, std::function<void()> callback) : RendererBase(ctx, callback) {}
    ~MainMenuRenderer() {VulkanCleanup();}

    void OnImGui() override;

  private:
    VkRenderPass getImGuiRenderPass() const override
    {
        return RenderPass;
    }

    void CreateResources() override;
    void CreateSwapchainResources() override;
    void CreateDependentResources() override;

    void DestroyResources() override;
    void DestroySwapchainResources() override;

    void SubmitCommandBuffers() override;

  private:
    void CreateRenderPasses();
    //void CreateGraphicsPipelines();

    void CreateCommandPools();
    void CreateCommandBuffers();
    void CreateFramebuffers();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

  private:
    VkRenderPass RenderPass;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
};