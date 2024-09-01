#include "VulkanContext.h"

VulkanContext::VulkanContext(uint32_t width, uint32_t height, std::string title) : Window(width, height, title)
{
    // Initialization done using vk-bootstrap, docs available at
    // https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md

    // Instance creation:
    auto inst_ret = vkb::InstanceBuilder()
                        .set_app_name(title.c_str())
                        .set_engine_name("No Engine")
                        .require_api_version(1, 0, 0)
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();

    if (!inst_ret)
        throw std::runtime_error(inst_ret.error().message());

    Instance = inst_ret.value();
    InstDisp = Instance.make_table();
    Surface = Window.CreateSurface(Instance);

    // Device selection:

    // To request required/desired device extensions:
    //.add_required_extension("VK_KHR_timeline_semaphore");
    //.add_desired_extension("VK_KHR_imageless_framebuffer");

    // To set required device features:
    // VkPhysicalDeviceFeatures required_features{};
    // required_features.multiViewport = true;
    // And then:
    // .set_required_features(required_features);

    auto phys_device_ret = vkb::PhysicalDeviceSelector(Instance).set_surface(Surface).select();

    if (!phys_device_ret)
        throw std::runtime_error(phys_device_ret.error().message());

    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    auto device_ret = vkb::DeviceBuilder(physical_device).build();

    if (!device_ret)
        throw std::runtime_error(device_ret.error().message());

    Device = device_ret.value();
    Disp = Device.make_table();

    // Swapchain creation:
    CreateSwapchain();
}

void VulkanContext::CreateSwapchain()
{
    // To manually specify format and present mode:
    //.set_desired_format(VkSurfaceFormatKHR)
    //.set_desired_present_mode(VkPresentModeKHR)

    auto swap_ret = vkb::SwapchainBuilder(Device).set_old_swapchain(Swapchain).build();

    if (!swap_ret)
        throw std::runtime_error(swap_ret.error().message());

    vkb::destroy_swapchain(Swapchain);

    Swapchain = swap_ret.value();
}