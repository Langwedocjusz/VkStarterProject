#pragma once

#include "ImGuiContext.h"
#include "VulkanContext.h"

#include "RendererBase.h"

#include <chrono>
#include <memory>

class Application {
  public:
    Application();
    ~Application();

    void Run();

    void OnResize(uint32_t width, uint32_t height);

  private:
    void RecreateRenderer(bool first_run = false);

  private:
    VulkanContext m_Ctx;

    enum class SupportedRenderer
    {
        MainMenu,
        HelloTraingle,
        TexturedQuad,
        TexturedCube,
        ComputeParticle,
        Model,
    };

    SupportedRenderer m_RendererType = SupportedRenderer::MainMenu;
    bool m_RecreateRenderer = true;

    std::unique_ptr<RendererBase> m_Renderer = nullptr;

    ImGuiContextManager m_ImGuiCtx;

    float mDeltaTime = 0.0f;
    std::chrono::time_point<std::chrono::high_resolution_clock> mOldTime;
};