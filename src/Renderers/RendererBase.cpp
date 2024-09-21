#include "RendererBase.h"

#include <iostream>

RendererBase::RendererBase(VulkanContext &context, std::function<void()> cb)
    : ctx(context), callback(cb)
{
    CreateQueues();
    CreateSyncObjects();
    CreateSwapchainViews();
}

RendererBase::~RendererBase()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        ctx.Disp.destroySemaphore(mRenderCompletedSemaphores[i], nullptr);
        ctx.Disp.destroySemaphore(mImageAcquiredSemaphores[i], nullptr);
        ctx.Disp.destroyFence(mInFlightFences[i], nullptr);
    }

    DestroySwapchainViews();
}

void RendererBase::OnUpdate()
{
}

void RendererBase::OnImGui()
{
}

void RendererBase::OnRender()
{
    DrawFrame();
    PresentFrame();
}

RenderDataForImGui RendererBase::getImGuiData() const
{
    return RenderDataForImGui{.Queue = mGraphicsQueue,
                              .FramesInFlight =
                                  static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
                              // Temporary, will need to actually fetch it from
                              // derived class:
                              .MSAA = VK_SAMPLE_COUNT_1_BIT};
}

void RendererBase::CreateQueues()
{
    auto gq = ctx.Device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value())
    {
        auto err_msg = "Failed to get graphics queue: " + gq.error().message();
        throw std::runtime_error(err_msg);
    }
    mGraphicsQueue = gq.value();

    auto pq = ctx.Device.get_queue(vkb::QueueType::present);
    if (!pq.has_value())
    {
        auto err_msg = "Failed to get present queue: " + pq.error().message();
        throw std::runtime_error(err_msg);
    }
    mPresentQueue = pq.value();
}

void RendererBase::CreateSyncObjects()
{
    mImageAcquiredSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderCompletedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (ctx.Disp.createSemaphore(&semaphore_info, nullptr,
                                     &mImageAcquiredSemaphores[i]) != VK_SUCCESS ||
            ctx.Disp.createSemaphore(&semaphore_info, nullptr,
                                     &mRenderCompletedSemaphores[i]) != VK_SUCCESS ||
            ctx.Disp.createFence(&fence_info, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects");
    }
}

void RendererBase::CreateSwapchainViews()
{
    mSwapchainImages = ctx.Swapchain.get_images().value();
    mSwapchainImageViews = ctx.Swapchain.get_image_views().value();
}

void RendererBase::DestroySwapchainViews()
{
    ctx.Swapchain.destroy_image_views(mSwapchainImageViews);
}

void RendererBase::DrawFrame()
{
    ctx.Disp.waitForFences(1, &mInFlightFences[mFrameSemaphoreIndex], VK_TRUE,
                           UINT64_MAX);

    auto &imageAcquiredSemaphore = mImageAcquiredSemaphores[mFrameSemaphoreIndex];

    if (ctx.SwapchainOk)
    {
        VkResult result = ctx.Disp.acquireNextImageKHR(ctx.Swapchain, UINT64_MAX,
                                                       imageAcquiredSemaphore,
                                                       VK_NULL_HANDLE, &mFrameImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            ctx.SwapchainOk = false;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }
    }

    if (!ctx.SwapchainOk)
        return;

    ctx.Disp.resetFences(1, &mInFlightFences[mFrameSemaphoreIndex]);

    SubmitCommandBuffers();
}

void RendererBase::PresentFrame()
{
    if (!ctx.SwapchainOk)
        return;

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signal_semaphores[] = {mRenderCompletedSemaphores[mFrameSemaphoreIndex]};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapChains[] = {ctx.Swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;

    present_info.pImageIndices = &mFrameImageIndex;

    VkResult result = ctx.Disp.queuePresentKHR(mPresentQueue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        ctx.SwapchainOk = false;
        return;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    mFrameSemaphoreIndex = (mFrameSemaphoreIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererBase::RecreateSwapchain()
{
    ctx.Disp.deviceWaitIdle();

    DestroySwapchainResources();
    DestroySwapchainViews();

    ctx.CreateSwapchain(ctx.Width, ctx.Height);
    CreateSwapchainViews();
    CreateSwapchainResources();

    ctx.SwapchainOk = true;

    // DrawFrame();
    // PresentFrame();
}