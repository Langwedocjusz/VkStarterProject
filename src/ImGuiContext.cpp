#include "ImGuiContext.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Renderer.h"

void ImGuiContext::OnInit(VulkanContext &ctx, RenderData &data)
{
    CreateDescriptorPool(ctx);
    InitImGui();
    InitImGuiVulkanBackend(ctx, data);
}

void ImGuiContext::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
}

void ImGuiContext::OnDestroy(VulkanContext &ctx)
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(ctx.Device, m_ImguiPool, nullptr);
}

void ImGuiContext::BeginGuiFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiContext::FinalizeGuiFrame()
{
    ImGui::Render();
}

void ImGuiContext::RecordImguiToCommandBuffer(VkCommandBuffer commandBuffer)
{
    ImDrawData *draw_data = ImGui::GetDrawData();

    if (draw_data)
        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void ImGuiContext::CreateDescriptorPool(VulkanContext &ctx)
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    auto res = vkCreateDescriptorPool(ctx.Device, &pool_info, nullptr, &m_ImguiPool);

    if (res != VK_SUCCESS)
        throw std::runtime_error("Failed to create Imgui Descriptor Pool!");
}

[[maybe_unused]] static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void ImGuiContext::InitImGuiVulkanBackend(VulkanContext &ctx, RenderData &data)
{
    ImGui_ImplGlfw_InitForVulkan(ctx.Window.get(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};

    init_info.Instance = ctx.Instance;
    init_info.PhysicalDevice = ctx.PhysicalDevice;
    init_info.Device = ctx.Device;

    // init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = data.GraphicsQueue;
    // init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = m_ImguiPool;
    init_info.RenderPass = data.RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = renderer::MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = renderer::MAX_FRAMES_IN_FLIGHT;
    init_info.MSAASamples =
        VK_SAMPLE_COUNT_1_BIT; // to-do: set this in ctx/data and retrieve from there
    // init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&init_info);
}