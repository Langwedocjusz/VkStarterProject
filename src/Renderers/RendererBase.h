#pragma once

#include "VulkanContext.h"

#include <functional>

/// Struct containing data needed for ImGuiContext
struct RenderDataForImGui {
    VkQueue Queue;
    uint32_t FramesInFlight;
    VkSampleCountFlagBits MSAA;
};

/**
    Abstract base class defining an interface for
    renderers to communicate with Application and ImGuiContext.
    Also implements basic behaviour for Queues/SyncObjects generation,
    frame drawing and presentation and swapchain recreation
    logic that can be overriden later.
*/
class RendererBase {
  public:
    RendererBase(VulkanContext &context, std::function<void()> cb);

    virtual ~RendererBase();

    virtual void OnUpdate();
    virtual void OnImGui();
    void OnRender();

    void RecreateSwapchain();

    RenderDataForImGui getImGuiData() const;

  protected:
    virtual void CreateSwapchainResources() = 0;
    virtual void DestroySwapchainResources() = 0;

    virtual void SubmitCommandBuffers() = 0;

    void CreateQueues();
    void CreateSyncObjects();
    void CreateSwapchainViews();
    void DestroySwapchainViews();

    void DrawFrame();
    void PresentFrame();

    void VulkanCleanup();

  protected:
    VulkanContext &ctx;
    std::function<void()> callback;

    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    VkQueue mGraphicsQueue;
    VkQueue mPresentQueue;

    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainImageViews;

    std::vector<VkSemaphore> mImageAcquiredSemaphores;
    std::vector<VkSemaphore> mRenderCompletedSemaphores;

    std::vector<VkFence> mInFlightFences;

    size_t mFrameSemaphoreIndex = 0;
    uint32_t mFrameImageIndex = 0;
};