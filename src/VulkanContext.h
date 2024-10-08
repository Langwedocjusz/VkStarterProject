#pragma once

#include "SystemWindow.h"
#include "VkBootstrap.h"

#include "vk_mem_alloc.h"

/**
    Class encapsulating elements of Vulkan application
    that will typically be present during the whole lifetime of
    the application i.e. vulkan instance, physical and logical device
    and System Window with the associated swapchain and surface.
*/
class VulkanContext {
  public:
    VulkanContext(uint32_t initial_width, uint32_t initial_height, std::string title,
                  void *usr_ptr = nullptr);
    ~VulkanContext();

    void CreateSwapchain(uint32_t width, uint32_t height, bool first_run = false);

  public:
    SystemWindow Window;

    vkb::Instance Instance;
    vkb::PhysicalDevice PhysicalDevice;
    vkb::Device Device;

    VmaAllocator Allocator;

    VkSurfaceKHR Surface;
    vkb::Swapchain Swapchain;

    std::vector<VkImage> SwapchainImages;
    std::vector<VkImageView> SwapchainImageViews;

    bool SwapchainOk = true;
    uint32_t Width;
    uint32_t Height;
};