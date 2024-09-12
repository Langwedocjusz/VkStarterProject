#pragma once

#include "ImGuiContext.h"
#include "VulkanContext.h"

#include "HelloTriangle.h"

class Application {
  public:
    Application();
    ~Application();

    void Run();

    void OnResize(uint32_t width, uint32_t height);

  private:
    VulkanContext m_Ctx;
    HelloTriangleRenderer m_Renderer;

    ImGuiContextManager m_ImGuiCtx;
};