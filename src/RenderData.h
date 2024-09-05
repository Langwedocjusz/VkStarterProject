#pragma once

#include <GLFW/glfw3.h>
#include <vector>

struct RenderData {
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;

    std::vector<VkImage> SwapchainImages;
    std::vector<VkImageView> SwapchainImageViews;
    std::vector<VkFramebuffer> Framebuffers;

    VkRenderPass RenderPass;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    std::vector<VkSemaphore> AvailableSemaphores;
    std::vector<VkSemaphore> FinishedSemaphores;
    std::vector<VkFence> InFlightFences;
    std::vector<VkFence> ImageInFlight;

    size_t CurrentFrame = 0;
    bool FramebufferResized = false;
};