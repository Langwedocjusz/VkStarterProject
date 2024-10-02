#include "RendererBase.h"

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "Utils.h"

RendererBase::RendererBase(VulkanContext &context, std::function<void()> cb)
    : ctx(context), callback(cb)
{
    // Create queues
    mGraphicsQueue = utils::GetQueue(ctx, vkb::QueueType::graphics);
    mPresentQueue = utils::GetQueue(ctx, vkb::QueueType::present);

    // Create base sync objects
    mImageAcquiredSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderCompletedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        utils::CreateSemaphore(ctx, mImageAcquiredSemaphores[i]);
        utils::CreateSemaphore(ctx, mRenderCompletedSemaphores[i]);
        utils::CreateSignalledFence(ctx, mInFlightFences[i]);
    }

    mMainDeletionQueue.push_back([&]() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            ctx.Disp.destroySemaphore(mRenderCompletedSemaphores[i], nullptr);
            ctx.Disp.destroySemaphore(mImageAcquiredSemaphores[i], nullptr);
            ctx.Disp.destroyFence(mInFlightFences[i], nullptr);
        }
    });
}

RendererBase::~RendererBase()
{
}

void RendererBase::OnUpdate([[maybe_unused]] float deltatime)
{
}

void RendererBase::OnImGui()
{
}

void RendererBase::OnRender()
{
    OnRenderImpl();
    mFrameSemaphoreIndex = (mFrameSemaphoreIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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

void RendererBase::RecreateSwapchain()
{
    ctx.Disp.deviceWaitIdle();

    mSwapchainDeletionQueue.flush();
    ctx.CreateSwapchain(ctx.Width, ctx.Height);
    CreateSwapchainResources();

    ctx.SwapchainOk = true;

    // OnRender();
}