#pragma once

// clang-format off
// Vulkan MUST be included before glfw:
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <cstdint>
#include <string>

class SystemWindow {
  public:
    SystemWindow(uint32_t width, uint32_t height, std::string title,
                 void *usr_ptr = nullptr);
    ~SystemWindow();

    bool ShouldClose();

    void PollEvents();
    void WaitEvents();

    GLFWwindow *get()
    {
        return m_Window;
    }

    VkSurfaceKHR CreateSurface(VkInstance instance,
                               VkAllocationCallbacks *allocator = nullptr);

  private:
    GLFWwindow *m_Window = nullptr;
};