#include "Application.h"

#include "Renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

static void InitImGui(VulkanContext &ctx, RenderData &data, VkDescriptorPool &imgui_pool)
{
    // Create descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    auto res = vkCreateDescriptorPool(ctx.Device, &pool_info, nullptr, &imgui_pool);

    if (res != VK_SUCCESS)
        throw std::runtime_error("Failed to create Imgui Descriptor Pool!");

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    //Note: currently colors are too bright because of color correction being applied twice
    //in ImGui itself and in Vulkan
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Init backends:
    ImGui_ImplGlfw_InitForVulkan(ctx.Window.get(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};

    init_info.Instance = ctx.Instance;
    init_info.PhysicalDevice = ctx.PhysicalDevice;
    init_info.Device = ctx.Device;

    // init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = data.GraphicsQueue;
    // init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = imgui_pool;
    init_info.RenderPass = data.RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;                   // to-do: retrieve this from ctx/data
    init_info.ImageCount = 2;                      // to-do: retrieve this from ctx/data
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // to-do: retrieve this from ctx/data
    // init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&init_info);
}

Application::Application() : m_Ctx(800, 600, "Vulkan", static_cast<void *>(this))
{
    renderer::CreateQueues(m_Ctx, m_Data);
    renderer::CreateRenderPass(m_Ctx, m_Data);
    renderer::CreateGraphicsPipeline(m_Ctx, m_Data);

    renderer::CreateFramebuffers(m_Ctx, m_Data);
    renderer::CreateCommandPool(m_Ctx, m_Data);
    renderer::CreateCommandBuffers(m_Ctx, m_Data);

    renderer::CreateSyncObjects(m_Ctx, m_Data);

    InitImGui(m_Ctx, m_Data, m_ImguiPool);
}

Application::~Application()
{
    m_Ctx.Disp.deviceWaitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // to-do: move this
    vkDestroyDescriptorPool(m_Ctx.Device, m_ImguiPool, nullptr);

    renderer::VulkanCleanup(m_Ctx, m_Data);
}

void Application::Run()
{
    static bool show_demo_window = true;

    while (!m_Ctx.Window.ShouldClose())
    {
        // Swapchain logic based on:
        // https://gist.github.com/nanokatze/bb03a486571e13a7b6a8709368bd87cf#file-handling-window-resize-md

        if (m_Ctx.SwapchainOk)
        {
            m_Ctx.Window.PollEvents();
        }
        else
        {
            m_Ctx.Window.WaitEvents();
            continue;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if(show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Render();

        renderer::DrawFrame(m_Ctx, m_Data);
        renderer::PresentFrame(m_Ctx, m_Data);
    }
}

void Application::OnResize(uint32_t width, uint32_t height)
{
    m_Ctx.SwapchainOk = false;
    m_Ctx.Width = width;
    m_Ctx.Height = height;

    renderer::RecreateSwapchain(m_Ctx, m_Data);
}