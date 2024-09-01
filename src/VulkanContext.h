#pragma once

#include "VkBootstrap.h"
#include <GLFW/glfw3.h>

class Window {
  public:
    Window(uint32_t width, uint32_t height, std::string title);
    ~Window();

    bool ShouldClose();
    void PollEvents();
    VkSurfaceKHR CreateSurface(VkInstance instance, VkAllocationCallbacks *allocator = nullptr);

  private:
    GLFWwindow *m_Window = nullptr;
};

struct VulkanContext {
    VulkanContext(uint32_t width, uint32_t height, std::string title);

    void CreateSwapchain();

    Window window;

    vkb::Instance instance;
    vkb::InstanceDispatchTable inst_disp;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::DispatchTable disp;
    vkb::Swapchain swapchain;
};