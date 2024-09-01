#include "VulkanContext.h"

#include <iostream>

Window::Window(uint32_t width, uint32_t height, std::string title)
{
    auto error_callback = [](int error, const char *description) {
        (void)error;
        std::cerr << "Glfw Error: " << description << '\n';
    };

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize glfw!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!m_Window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create a window!");
    }
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(m_Window);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance, VkAllocationCallbacks *allocator)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(instance, m_Window, allocator, &surface);
    if (err)
        throw std::runtime_error("Failed to create a surface!");

    return surface;
}

VulkanContext::VulkanContext(uint32_t width, uint32_t height, std::string title) : window(width, height, title)
{
    // Device creation:
    vkb::InstanceBuilder builder;
    auto inst_ret =
        builder.set_app_name(title.c_str()).request_validation_layers().use_default_debug_messenger().build();
    if (!inst_ret)
        throw std::runtime_error(inst_ret.error().message());

    instance = inst_ret.value();
    inst_disp = instance.make_table();
    surface = window.CreateSurface(instance);

    vkb::PhysicalDeviceSelector phys_device_selector(instance);

    auto phys_device_ret = phys_device_selector.set_surface(surface).select();
    if (!phys_device_ret)
        throw std::runtime_error(phys_device_ret.error().message());

    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    vkb::DeviceBuilder device_builder{physical_device};

    auto device_ret = device_builder.build();
    if (!device_ret)
        throw std::runtime_error(device_ret.error().message());

    device = device_ret.value();
    disp = device.make_table();

    // Swapchain creation:
    CreateSwapchain();
}

void VulkanContext::CreateSwapchain()
{
    vkb::SwapchainBuilder swapchain_builder{device};

    auto swap_ret = swapchain_builder.set_old_swapchain(swapchain).build();
    if (!swap_ret)
        throw std::runtime_error(swap_ret.error().message());

    vkb::destroy_swapchain(swapchain);
    swapchain = swap_ret.value();
}