#pragma once

#include "VulkanContext.h"

/// Struct containing data needed for ImGuiContext
struct RenderDataForImGui {
    VkQueue Queue;
    VkRenderPass RenderPass;
    uint32_t FramesInFlight;
    VkSampleCountFlagBits MSAA;
};

/**
    Abstract base class defining an interface for
    renderers to communicate with Application and ImGuiContext.
    Also implements basic behaviour for Queues/SyncObjects/Framebuffers
    generation, frame drawing and presentation and swapchain recreation
    logic that can be overriden later.
*/
class RendererBase {
  public:
    virtual ~RendererBase();

    void OnInit(VulkanContext &ctx);

    virtual void OnUpdate();
    virtual void OnImGui();
    void OnRender(VulkanContext &ctx);

    void RecreateSwapchain(VulkanContext &ctx);

    void VulkanCleanup(VulkanContext &ctx);

    RenderDataForImGui getImGuiData() const;

  protected:
    virtual VkRenderPass getImGuiRenderPass() const = 0;

    virtual void CreateResources(VulkanContext &ctx) = 0;
    virtual void CreateDependentResources(VulkanContext &ctx) = 0;
    virtual void DestroyResources(VulkanContext &ctx) = 0;

    virtual void CreateSwapchainResources(VulkanContext &ctx) = 0;
    virtual void DestroySwapchainResources(VulkanContext &ctx) = 0;

    virtual void SubmitCommandBuffers(VulkanContext &ctx) = 0;

    void CreateQueues(VulkanContext &ctx);
    void CreateSyncObjects(VulkanContext &ctx);
    void CreateSwapchainViews(VulkanContext &ctx);
    void DestroySwapchainViews(VulkanContext &ctx);

    void DrawFrame(VulkanContext &ctx);
    void PresentFrame(VulkanContext &ctx);

  protected:
    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    VkQueue GraphicsQueue;
    VkQueue PresentQueue;

    std::vector<VkImage> SwapchainImages;
    std::vector<VkImageView> SwapchainImageViews;
    std::vector<VkFramebuffer> Framebuffers;

    std::vector<VkSemaphore> ImageAcquiredSemaphores;
    std::vector<VkSemaphore> RenderCompletedSemaphores;

    std::vector<VkFence> InFlightFences;

    size_t FrameSemaphoreIndex = 0;
    uint32_t FrameImageIndex = 0;
};