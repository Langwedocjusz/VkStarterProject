#pragma once

#include <string>
#include <vector>

#include "VulkanContext.h"

namespace utils
{
std::vector<char> ReadFileBinary(const std::string &filename);

VkShaderModule CreateShaderModule(VulkanContext &ctx, const std::vector<char> &code);

void CreateBuffer(VulkanContext &ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);

struct CopyBufferInfo{
    VkQueue Queue;
    VkCommandPool Pool;
    VkBuffer Src;
    VkBuffer Dst;
    VkDeviceSize Size;
};

void CopyBuffer(VulkanContext &ctx, CopyBufferInfo info);
} // namespace utils