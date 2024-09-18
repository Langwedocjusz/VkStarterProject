#pragma once

#include "VulkanContext.h"

#include "Buffer.h"

//Currently assumes 2d image without mips
struct ImageInfo {
    uint32_t Width;
    uint32_t Height;
    VkFormat Format;
    VkImageTiling Tiling;
    VkImageUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

struct ImageDataInfo{
    VkQueue Queue;
    VkCommandPool Pool;
    const void *Data;
    VkDeviceSize Size;
};

class Image {
  public:
    Image() = default;

    static Image CreateImage(VulkanContext &ctx, ImageInfo info);
    static void DestroyImage(VulkanContext &ctx, Image &img);

    static void UploadToImage(VulkanContext &ctx, Image& img, ImageDataInfo info);

  public:
    VkImage Handle;
    VkDeviceMemory Memory;
    ImageInfo Info;
};