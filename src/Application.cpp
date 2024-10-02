#include "Application.h"

#include "ComputeParticles.h"
#include "HelloTriangle.h"
#include "MainMenu.h"
#include "Model.h"
#include "TexturedCube.h"
#include "TexturedQuad.h"

#include "imgui.h"

Application::Application() : m_Ctx(800, 600, "Vulkanik", static_cast<void *>(this))
{
    RecreateRenderer(true);
    m_RecreateRenderer = false;
}

Application::~Application()
{
    // Imgui destructed ad the end of Run, destructors of renderer and ctx clean up the
    // rest
}

void Application::Run()
{
    while (!m_Ctx.Window.ShouldClose())
    {
        if (m_RecreateRenderer)
        {
            RecreateRenderer();
            m_RecreateRenderer = false;
        }

        // Swapchain logic based on:
        // https://gist.github.com/nanokatze/bb03a486571e13a7b6a8709368bd87cf#file-handling-window-resize-md
        m_Renderer->OnUpdate();

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
        m_Renderer->OnImGui();
        m_ImGuiCtx.FinalizeGuiFrame();

        m_Renderer->OnRender();
    }

    m_Ctx.Disp.deviceWaitIdle();
    // Here not in the destructor, to avoid triggering when an exception is thrown
    // as that results in imgui assert preventing the exception from propagating
    // up and being printed to cerr.
    m_ImGuiCtx.OnDestroy(m_Ctx);
}

void Application::OnResize(uint32_t width, uint32_t height)
{
    m_Ctx.SwapchainOk = false;
    m_Ctx.Width = width;
    m_Ctx.Height = height;

    m_Renderer->RecreateSwapchain();
}

void Application::RecreateRenderer(bool first_run)
{
    using enum SupportedRenderer;

    if (!first_run)
    {
        m_Ctx.Disp.deviceWaitIdle();
        m_ImGuiCtx.OnDestroy(m_Ctx);
    }

    auto go_back = [this]() {
        if (ImGui::ArrowButton("Go back", static_cast<ImGuiDir>(0)))
        {
            m_RecreateRenderer = true;
            m_RendererType = MainMenu;
        }

        ImGui::SameLine();
        ImGui::Text("Go back");

        ImGui::Separator();
    };

    auto menu = [this]() {
        auto size = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);

        if (ImGui::Button("Hello Triangle", size))
        {
            m_RecreateRenderer = true;
            m_RendererType = HelloTraingle;
        }
        if (ImGui::Button("Textured Quad", size))
        {
            m_RecreateRenderer = true;
            m_RendererType = TexturedQuad;
        }
        if (ImGui::Button("Textured Cube", size))
        {
            m_RecreateRenderer = true;
            m_RendererType = TexturedCube;
        }
        if (ImGui::Button("Compute Particles", size))
        {
            m_RecreateRenderer = true;
            m_RendererType = ComputeParticle;
        }
        if (ImGui::Button("Model", size))
        {
            m_RecreateRenderer = true;
            m_RendererType = Model;
        }
    };

    switch (m_RendererType)
    {
    // Note that destructors cleaning up vulkan resources are
    // called automatically
    case MainMenu: {
        m_Renderer = std::make_unique<MainMenuRenderer>(m_Ctx, menu);
        break;
    }
    case HelloTraingle: {
        m_Renderer = std::make_unique<HelloTriangleRenderer>(m_Ctx, go_back);
        break;
    }
    case TexturedQuad: {
        m_Renderer = std::make_unique<TexturedQuadRenderer>(m_Ctx, go_back);
        break;
    }
    case TexturedCube: {
        m_Renderer = std::make_unique<TexturedCubeRenderer>(m_Ctx, go_back);
        break;
    }
    case ComputeParticle: {
        m_Renderer = std::make_unique<ComputeParticleRenderer>(m_Ctx, go_back);
        break;
    }
    case Model: {
        m_Renderer = std::make_unique<ModelRenderer>(m_Ctx, go_back);
        break;
    }
    }

    m_ImGuiCtx.OnInit(m_Ctx, m_Renderer.get());
}