#pragma once

#include "RendererBase.h"
#include "VulkanContext.h"

class ImGuiContextManager {
  public:
    void OnInit(VulkanContext &ctx, const RendererBase *const renderer);
    void OnDestroy(VulkanContext &ctx);

    void BeginGuiFrame();
    void FinalizeGuiFrame();

    static void RecordImguiToCommandBuffer(VkCommandBuffer commandBuffer);

  private:
    VkDescriptorPool m_ImguiPool;

    void InitImGui();
    void CreateDescriptorPool(VulkanContext &ctx);
    void InitImGuiVulkanBackend(VulkanContext &ctx, const RendererBase *const renderer);
};