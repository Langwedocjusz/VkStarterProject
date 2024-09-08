#pragma once

#include <string>
#include <vector>

#include "VulkanContext.h"

namespace utils
{
std::vector<char> ReadFileBinary(const std::string &filename);
VkShaderModule CreateShaderModule(VulkanContext &ctx, const std::vector<char> &code);
} // namespace utils