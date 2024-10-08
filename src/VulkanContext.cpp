#include "VulkanContext.h"

VulkanContext::VulkanContext(uint32_t width, uint32_t height, std::string title,
                             void *usr_ptr)
    : Window(width, height, title, usr_ptr)
{
    // Initialization done using vk-bootstrap, docs available at
    // https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md

    // Instance creation:
    auto inst_ret = vkb::InstanceBuilder()
                        .set_app_name(title.c_str())
                        .set_engine_name("No Engine")
                        .require_api_version(1, 3, 0)
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

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = true;

    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;

    auto phys_device_ret = vkb::PhysicalDeviceSelector(Instance)
                               .set_surface(Surface)
                               .set_required_features(features)
                               .set_required_features_13(features13)
                               .select();

    if (!phys_device_ret)
        throw std::runtime_error(phys_device_ret.error().message());

    PhysicalDevice = phys_device_ret.value();

    auto device_ret = vkb::DeviceBuilder(PhysicalDevice).build();

    if (!device_ret)
        throw std::runtime_error(device_ret.error().message());

    Device = device_ret.value();

    // Swapchain creation:
    CreateSwapchain(width, height, true);
}

VulkanContext::~VulkanContext()
{
    Swapchain.destroy_image_views(SwapchainImageViews);

    vkb::destroy_swapchain(Swapchain);
    vkb::destroy_device(Device);
    vkb::destroy_surface(Instance, Surface);
    vkb::destroy_instance(Instance);
}

void VulkanContext::CreateSwapchain(uint32_t width, uint32_t height, bool first_run)
{
    if (!first_run)
        Swapchain.destroy_image_views(SwapchainImageViews);

    // To manually specify format and present mode:
    //.set_desired_format(VkSurfaceFormatKHR)
    //.set_desired_present_mode(VkPresentModeKHR)
    auto swap_ret = vkb::SwapchainBuilder(Device)
                        .set_old_swapchain(Swapchain)
                        .set_desired_extent(width, height)
                        .build();

    if (!swap_ret)
        throw std::runtime_error(swap_ret.error().message());

    vkb::destroy_swapchain(Swapchain);

    Swapchain = swap_ret.value();

    SwapchainImages = Swapchain.get_images().value();
    SwapchainImageViews = Swapchain.get_image_views().value();
}