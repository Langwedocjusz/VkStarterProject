#pragma once

#include "RenderData.h"
#include "VulkanContext.h"

class Application {
  public:
    Application();
    ~Application();

    void Run();

    void OnResize(uint32_t width, uint32_t height);

  private:
    VulkanContext m_Ctx;
    RenderData m_Data;
};