#pragma once

#include <string>

#include "VulkanContext.h"

class ShaderBuilder {
  public:
    ShaderBuilder() = default;

    ShaderBuilder SetVertexPath(std::string_view path)
    {
        mVertexPath = path;
        return *this;
    }
    ShaderBuilder SetFragmentPath(std::string_view path)
    {
        mFragmentPath = path;
        return *this;
    }

    std::vector<VkPipelineShaderStageCreateInfo> Build(VulkanContext &ctx);

  private:
    std::string mVertexPath;
    std::string mFragmentPath;
};