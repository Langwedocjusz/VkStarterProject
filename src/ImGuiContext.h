#pragma once

#include "RenderData.h"
#include "VulkanContext.h"

class ImGuiContext {
  public:
    ImGuiContext() = default;

    void OnInit(VulkanContext &ctx, RenderData &data);
    void OnDestroy(VulkanContext &ctx);

    void BeginGuiFrame();
    void FinalizeGuiFrame();

    static void RecordImguiToCommandBuffer(VkCommandBuffer commandBuffer);

  private:
    VkDescriptorPool m_ImguiPool;

    void InitImGui();
    void CreateDescriptorPool(VulkanContext &ctx);
    void InitImGuiVulkanBackend(VulkanContext &ctx, RenderData &data);
};