#pragma once

#include "VulkanContext.h"

#include "Buffer.h"

struct ImageInfo {
    uint32_t Width;
    uint32_t Height;
    VkFormat Format;
    VkImageTiling Tiling;
    VkImageUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

class Image {
  public:
    Image() = default;

    static Image CreateImage(VulkanContext &ctx, ImageInfo info);
    static void DestroyImage(VulkanContext &ctx, Image &img);

  public:
    VkImage Handle;
    VkDeviceMemory Memory;
};