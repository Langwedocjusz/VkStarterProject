#pragma once

#include "SystemWindow.h"
#include "VkBootstrap.h"

struct VulkanContext {
    VulkanContext(uint32_t width, uint32_t height, std::string title, void *usr_ptr = nullptr);

    void CreateSwapchain(uint32_t width, uint32_t height);

    SystemWindow Window;

    vkb::Instance Instance;
    vkb::InstanceDispatchTable InstDisp;
    VkSurfaceKHR Surface;
    vkb::PhysicalDevice PhysicalDevice;
    vkb::Device Device;
    vkb::DispatchTable Disp;
    vkb::Swapchain Swapchain;

    bool SwapchainOk = true;
    uint32_t Width;
    uint32_t Height;
};