#include "SystemWindow.h"

#include "Application.h"

#include <iostream>

static void FramebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));

    app->OnResize();

    (void)width;
    (void)height;
}

SystemWindow::SystemWindow(uint32_t width, uint32_t height, std::string title, void *usr_ptr)
{
    auto error_callback = [](int error, const char *description) {
        (void)error;
        std::cerr << "Glfw Error: " << description << '\n';
    };

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize glfw!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!m_Window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create a window!");
    }

    glfwSetWindowUserPointer(m_Window, usr_ptr);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
}

SystemWindow::~SystemWindow()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool SystemWindow::ShouldClose()
{
    return glfwWindowShouldClose(m_Window);
}

void SystemWindow::PollEvents()
{
    glfwPollEvents();
}

VkSurfaceKHR SystemWindow::CreateSurface(VkInstance instance, VkAllocationCallbacks *allocator)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(instance, m_Window, allocator, &surface);
    if (err)
        throw std::runtime_error("Failed to create a surface!");

    return surface;
}