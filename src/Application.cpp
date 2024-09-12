#include "Application.h"

Application::Application() : m_Ctx(800, 600, "Vulkanik", static_cast<void *>(this))
{
    m_Renderer.OnInit(m_Ctx);
    m_ImGuiCtx.OnInit(m_Ctx, m_Renderer);
}

Application::~Application()
{
    m_Ctx.Disp.deviceWaitIdle();

    m_ImGuiCtx.OnDestroy(m_Ctx);
    m_Renderer.VulkanCleanup(m_Ctx);
}

void Application::Run()
{
    while (!m_Ctx.Window.ShouldClose())
    {
        // Swapchain logic based on:
        // https://gist.github.com/nanokatze/bb03a486571e13a7b6a8709368bd87cf#file-handling-window-resize-md
        m_Renderer.OnUpdate();

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
        m_Renderer.OnImGui();
        m_ImGuiCtx.FinalizeGuiFrame();

        m_Renderer.OnRender(m_Ctx);
    }
}

void Application::OnResize(uint32_t width, uint32_t height)
{
    m_Ctx.SwapchainOk = false;
    m_Ctx.Width = width;
    m_Ctx.Height = height;

    m_Renderer.RecreateSwapchain(m_Ctx);
}