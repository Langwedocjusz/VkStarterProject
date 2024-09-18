#pragma once

#include "VulkanContext.h"

class Buffer {
  public:
    Buffer() = default;

    static uint32_t FindMemoryType(VulkanContext &ctx, uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties);

    static Buffer CreateBuffer(VulkanContext &ctx, VkDeviceSize size,
                               VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties);
    static void DestroyBuffer(VulkanContext &ctx, Buffer &buf);

  public:
    VkBuffer Handle;
    VkDeviceMemory Memory;
};