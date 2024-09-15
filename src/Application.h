#pragma once

#include "ImGuiContext.h"
#include "VulkanContext.h"

#include "RendererBase.h"

#include <memory>

class Application {
  public:
    Application();
    ~Application();

    void Run();

    void OnResize(uint32_t width, uint32_t height);

  private:
    VulkanContext m_Ctx;
    std::unique_ptr<RendererBase> m_Renderer = nullptr;

    ImGuiContextManager m_ImGuiCtx;
};