#pragma once

#include "RendererBase.h"

#include <glm/glm.hpp>

class MainMenuRenderer : public RendererBase {
  public:
    MainMenuRenderer(VulkanContext &ctx, std::function<void()> callback)
        : RendererBase(ctx, callback)
    {
    }
    ~MainMenuRenderer()
    {
        VulkanCleanup();
    }

    void OnImGui() override;

  private:
    void CreateResources() override;
    void CreateSwapchainResources() override;
    void CreateDependentResources() override;

    void DestroyResources() override;
    void DestroySwapchainResources() override;

    void SubmitCommandBuffers() override;

  private:
    void CreateCommandPools();
    void CreateCommandBuffers();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  private:
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
};