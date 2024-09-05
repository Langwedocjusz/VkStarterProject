#pragma once

#include "RenderData.h"
#include "VulkanContext.h"

class Application {
  public:
    Application();
    ~Application();

    void Run();

    void OnResize();
  private:
    const uint32_t m_Width = 800;
    const uint32_t m_Height = 600;
    const std::string m_Title = "Vulkan";

    VulkanContext m_Ctx;
    RenderData m_Data;
};