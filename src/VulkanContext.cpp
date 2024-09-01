#include "VulkanContext.h"

VulkanContext::VulkanContext(uint32_t width, uint32_t height, std::string title) : Window(width, height, title)
{
    // Device creation:
    vkb::InstanceBuilder builder;
    auto inst_ret =
        builder.set_app_name(title.c_str()).request_validation_layers().use_default_debug_messenger().build();
    if (!inst_ret)
        throw std::runtime_error(inst_ret.error().message());

    Instance = inst_ret.value();
    InstDisp = Instance.make_table();
    Surface = Window.CreateSurface(Instance);

    vkb::PhysicalDeviceSelector phys_device_selector(Instance);

    auto phys_device_ret = phys_device_selector.set_surface(Surface).select();
    if (!phys_device_ret)
        throw std::runtime_error(phys_device_ret.error().message());

    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    vkb::DeviceBuilder device_builder{physical_device};

    auto device_ret = device_builder.build();
    if (!device_ret)
        throw std::runtime_error(device_ret.error().message());

    Device = device_ret.value();
    Disp = Device.make_table();

    // Swapchain creation:
    CreateSwapchain();
}

void VulkanContext::CreateSwapchain()
{
    vkb::SwapchainBuilder swapchain_builder{Device};

    auto swap_ret = swapchain_builder.set_old_swapchain(Swapchain).build();
    if (!swap_ret)
        throw std::runtime_error(swap_ret.error().message());

    vkb::destroy_swapchain(Swapchain);
    Swapchain = swap_ret.value();
}