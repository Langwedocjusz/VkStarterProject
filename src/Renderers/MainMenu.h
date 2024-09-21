#pragma once

#include "RendererBase.h"

#include <glm/glm.hpp>

class MainMenuRenderer : public RendererBase {
  public:
    MainMenuRenderer(VulkanContext &ctx, std::function<void()> callback);

    ~MainMenuRenderer();

    void OnImGui() override;

  private:
    void CreateSwapchainResources() override;
    void DestroySwapchainResources() override;

    void SubmitCommandBuffers() override;

  private:
    void CreateCommandPools();
    void CreateCommandBuffers();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  private:
    VkCommandPool mCommandPool;
    std::vector<VkCommandBuffer> mCommandBuffers;
};