#include "MainMenu.h"

#include "Common.h"

#include "ImGuiContext.h"
#include "imgui.h"

MainMenuRenderer::MainMenuRenderer(VulkanContext &ctx, std::function<void()> callback)
    : RendererBase(ctx, callback)
{
    CreateSwapchainResources();
}

MainMenuRenderer::~MainMenuRenderer()
{
    DestroySwapchainResources();
}

void MainMenuRenderer::OnImGui()
{
    ImGui::Begin("Main Menu");
    callback();
    ImGui::End();
}

void MainMenuRenderer::OnRenderImpl()
{
    auto &imageAcquiredSemaphore = mImageAcquiredSemaphores[mFrameSemaphoreIndex];
    auto &renderCompleteSemaphore = mRenderCompletedSemaphores[mFrameSemaphoreIndex];
    auto &fence = mInFlightFences[mFrameSemaphoreIndex];

    vkWaitForFences(ctx.Device, 1, &fence, VK_TRUE, UINT64_MAX);

    common::AcquireNextImage(ctx, imageAcquiredSemaphore, mFrameImageIndex);

    if (!ctx.SwapchainOk)
        return;

    vkResetFences(ctx.Device, 1, &fence);

    // DrawFrame
    {
        auto &buffer = mCommandBuffers[mFrameSemaphoreIndex];

        vkResetCommandBuffer(buffer, 0);
        RecordCommandBuffer(buffer, mFrameImageIndex);

        auto buffers = std::array<VkCommandBuffer, 1>{buffer};

        common::SubmitGraphicsQueueDefault(mGraphicsQueue, buffers, fence,
                                           imageAcquiredSemaphore,
                                           renderCompleteSemaphore);
    }

    common::PresentFrame(ctx, mPresentQueue, renderCompleteSemaphore, mFrameImageIndex);
}

void MainMenuRenderer::CreateSwapchainResources()
{
    CreateCommandPools();
    CreateCommandBuffers();
}

void MainMenuRenderer::DestroySwapchainResources()
{
    vkDestroyCommandPool(ctx.Device, mCommandPool, nullptr);
}

void MainMenuRenderer::CreateCommandPools()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(ctx.Device, &pool_info, nullptr, &mCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");
}

void MainMenuRenderer::CreateCommandBuffers()
{
    mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, mCommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void MainMenuRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                           uint32_t imageIndex)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    common::ImageBarrierColorToRender(commandBuffer, ctx.SwapchainImages[imageIndex]);

    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = ctx.SwapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = {0, 0, ctx.Swapchain.extent.width,
                                ctx.Swapchain.extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    {
        common::ViewportScissorDefaultBehaviour(ctx, commandBuffer);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }
    vkCmdEndRendering(commandBuffer);

    common::ImageBarrierColorToPresent(commandBuffer, ctx.SwapchainImages[imageIndex]);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}