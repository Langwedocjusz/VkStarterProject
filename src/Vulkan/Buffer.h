#pragma once

#include "VulkanContext.h"

struct CopyBufferInfo {
    VkQueue Queue;
    VkCommandPool Pool;
    VkBuffer Src;
    VkBuffer Dst;
    VkDeviceSize Size;
};

struct GPUBufferInfo {
    VkQueue Queue;
    VkCommandPool Pool;
    const void *Data;
    VkDeviceSize Size;
    VkBufferUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

class Buffer {
  public:
    Buffer() = default;

    static uint32_t FindMemoryType(VulkanContext &ctx, uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties);

    static Buffer CreateBuffer(VulkanContext &ctx, VkDeviceSize size,
                               VkBufferUsageFlags usage, VmaAllocationCreateFlags flags);
    static void DestroyBuffer(VulkanContext &ctx, Buffer &buf);

    static void UploadToBuffer(VulkanContext &ctx, Buffer buff, const void *data,
                               VkDeviceSize size);

    static void UploadToMappedBuffer(Buffer buff, const void *data, VkDeviceSize size);

    static Buffer CreateStagingBuffer(VulkanContext &ctx, VkDeviceSize size);
    static Buffer CreateMappedUniformBuffer(VulkanContext &ctx, VkDeviceSize size);
    static Buffer CreateGPUBuffer(VulkanContext &ctx, GPUBufferInfo info);

    static void CopyBuffer(VulkanContext &ctx, CopyBufferInfo info);

  public:
    VkBuffer Handle;
    VmaAllocation Allocation;
    VmaAllocationInfo AllocInfo;
};