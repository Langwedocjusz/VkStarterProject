#include "Image.h"

#include "Buffer.h"
#include "Utils.h"

Image Image::CreateImage(VulkanContext &ctx, ImageInfo info)
{
    Image img;
    img.Info = info;

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

void Image::UploadToImage(VulkanContext &ctx, Image &img, ImageDataInfo info)
{
    Buffer stagingBuffer = Buffer::CreateStagingBuffer(ctx, info.Size);

    Buffer::UploadToBuffer(ctx, stagingBuffer, info.Data, info.Size);

    TransitionImageLayoutInfo trInfo{
        .Queue = info.Queue,
        .Pool = info.Pool,
        .Image = img.Handle,
        .Format = img.Info.Format,
        .OldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .NewLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    };

    TransitionImageLayout(ctx, trInfo);

    CopyBufferToImageInfo cpInfo{
        .Queue = info.Queue,
        .Pool = info.Pool,
        .Buffer = stagingBuffer.Handle,
        .Image = img.Handle,
        .Width = img.Info.Width,
        .Height = img.Info.Height,
    };

    CopyBufferToImage(ctx, cpInfo);

    trInfo.OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    trInfo.NewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    TransitionImageLayout(ctx, trInfo);

    Buffer::DestroyBuffer(ctx, stagingBuffer);
}

void Image::CopyBufferToImage(VulkanContext &ctx, CopyBufferToImageInfo info)
{
    utils::ScopedCommand cmd(ctx, info.Queue, info.Pool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {info.Width, info.Height, 1};

    vkCmdCopyBufferToImage(cmd.Buffer, info.Buffer, info.Image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void Image::TransitionImageLayout(VulkanContext &ctx, TransitionImageLayoutInfo info)
{
    utils::ScopedCommand cmd(ctx, info.Queue, info.Pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = info.OldLayout;
    barrier.newLayout = info.NewLayout;

    // Other values used to transfer ownership between queues:
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = info.Image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    bool undefined_to_dst = (info.OldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                             info.NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    bool dst_to_shader_read =
        (info.OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
         info.NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (undefined_to_dst)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (dst_to_shader_read)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(cmd.Buffer, sourceStage, destinationStage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
}