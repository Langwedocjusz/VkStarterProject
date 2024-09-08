#include "Application.h"

#include "Renderer.h"

#include "imgui.h"

Application::Application() : m_Ctx(800, 600, "Vulkanik", static_cast<void *>(this))
{
    renderer::CreateQueues(m_Ctx, m_Data);
    renderer::CreateRenderPass(m_Ctx, m_Data);
    renderer::CreateGraphicsPipeline(m_Ctx, m_Data);

    renderer::CreateFramebuffers(m_Ctx, m_Data);
    renderer::CreateCommandPool(m_Ctx, m_Data);
    renderer::CreateCommandBuffers(m_Ctx, m_Data);

    renderer::CreateSyncObjects(m_Ctx, m_Data);

    m_ImGuiCtx.OnInit(m_Ctx, m_Data);
}

Application::~Application()
{
    m_Ctx.Disp.deviceWaitIdle();
    m_ImGuiCtx.OnDestroy(m_Ctx);
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

        m_ImGuiCtx.BeginGuiFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        m_ImGuiCtx.FinalizeGuiFrame();

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