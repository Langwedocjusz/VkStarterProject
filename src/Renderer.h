#pragma once

#include "RenderData.h"
#include "VulkanContext.h"

namespace renderer
{
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

void CreateQueues(VulkanContext &ctx, RenderData &data);
void CreateRenderPass(VulkanContext &ctx, RenderData &data);
void CreateGraphicsPipeline(VulkanContext &ctx, RenderData &data);
void CreateSyncObjects(VulkanContext &ctx, RenderData &data);

void CreateFramebuffers(VulkanContext &ctx, RenderData &data);
void CreateCommandPool(VulkanContext &ctx, RenderData &data);
void CreateCommandBuffers(VulkanContext &ctx, RenderData &data);

void RecreateSwapchain(VulkanContext &ctx, RenderData &data);

void DrawFrame(VulkanContext &ctx, RenderData &data);

void VulkanCleanup(VulkanContext &ctx, RenderData &data);
}; // namespace renderer