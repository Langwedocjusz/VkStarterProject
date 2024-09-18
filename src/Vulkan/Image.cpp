#include "Image.h"

Image Image::CreateImage(VulkanContext &ctx, ImageInfo info)
{
    Image img;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = info.Width;
    imageInfo.extent.height = info.Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = info.Format;
    // Actual order of pixels in memory, not sampler tiling:
    imageInfo.tiling = info.Tiling;
    // Only other option is PREINITIALIZED:
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage = info.Usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Multisampling, only relevant for attachments:
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    if (vkCreateImage(ctx.Device, &imageInfo, nullptr, &img.Handle) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(ctx.Device, img.Handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        Buffer::FindMemoryType(ctx, memRequirements.memoryTypeBits, info.Properties);

    if (vkAllocateMemory(ctx.Device, &allocInfo, nullptr, &img.Memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory!");

    vkBindImageMemory(ctx.Device, img.Handle, img.Memory, 0);

    return img;
}

void Image::DestroyImage(VulkanContext &ctx, Image &img)
{
    vkDestroyImage(ctx.Device, img.Handle, nullptr);
    vkFreeMemory(ctx.Device, img.Memory, nullptr);
}