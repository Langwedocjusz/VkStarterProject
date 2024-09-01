#pragma once

#include "SystemWindow.h"
#include "VkBootstrap.h"

struct VulkanContext {
    VulkanContext(uint32_t width, uint32_t height, std::string title);

    void CreateSwapchain();

    SystemWindow Window;

    vkb::Instance Instance;
    vkb::InstanceDispatchTable InstDisp;
    VkSurfaceKHR Surface;
    vkb::Device Device;
    vkb::DispatchTable Disp;
    vkb::Swapchain Swapchain;

    bool FramebufferResized = false;
};