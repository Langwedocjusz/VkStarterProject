#include "Application.h"

#include "Renderer.h"

Application::Application() : m_Ctx(m_Width, m_Height, m_Title, static_cast<void*>(this))
{
    renderer::CreateQueues(m_Ctx, m_Data);
    renderer::CreateRenderPass(m_Ctx, m_Data);
    renderer::CreateGraphicsPipeline(m_Ctx, m_Data);

    renderer::CreateFramebuffers(m_Ctx, m_Data);
    renderer::CreateCommandPool(m_Ctx, m_Data);
    renderer::CreateCommandBuffers(m_Ctx, m_Data);

    renderer::CreateSyncObjects(m_Ctx, m_Data);
}

Application::~Application()
{
    m_Ctx.Disp.deviceWaitIdle();
    renderer::VulkanCleanup(m_Ctx, m_Data);
}

void Application::Run()
{
    while (!m_Ctx.Window.ShouldClose())
    {
        m_Ctx.Window.PollEvents();

        renderer::DrawFrame(m_Ctx, m_Data);
    }
}

void Application::OnResize()
{
    m_Data.FramebufferResized = true;
}