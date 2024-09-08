#pragma once

#include "Renderer.h"
#include "VulkanContext.h"

class ImGuiContext {
  public:
    void OnInit(VulkanContext &ctx, const Renderer &renderer);
    void OnDestroy(VulkanContext &ctx);

    void BeginGuiFrame();
    void FinalizeGuiFrame();

    static void RecordImguiToCommandBuffer(VkCommandBuffer commandBuffer);

  private:
    VkDescriptorPool m_ImguiPool;

    void InitImGui();
    void CreateDescriptorPool(VulkanContext &ctx);
    void InitImGuiVulkanBackend(VulkanContext &ctx, const Renderer &renderer);
};