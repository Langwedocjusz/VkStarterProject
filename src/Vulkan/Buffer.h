#pragma once

#include "VulkanContext.h"

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
                               VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties);
    static void DestroyBuffer(VulkanContext &ctx, Buffer &buf);

    static void UploadToBuffer(VulkanContext &ctx, Buffer buff, const void *data,
                               VkDeviceSize size);

    static Buffer CreateStagingBuffer(VulkanContext &ctx, VkDeviceSize size);
    static Buffer CreateMappedUniformBuffer(VulkanContext &ctx, VkDeviceSize size);
    static Buffer CreateGPUBuffer(VulkanContext &ctx, GPUBufferInfo info);

  public:
    VkBuffer Handle;
    VkDeviceMemory Memory;
};

class MappedUniformBuffer{
public:
    MappedUniformBuffer() = default;

    void OnInit(VulkanContext &ctx, VkDeviceSize size);
    void OnDestroy(VulkanContext &ctx);

    void UploadData(const void* data, size_t size);

    VkBuffer Handle() const {return mBuffer.Handle;}
private:
    Buffer mBuffer;
    void* mData = nullptr;
};