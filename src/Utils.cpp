#include "Utils.h"

#include <fstream>
#include <iostream>

std::vector<char> utils::ReadFileBinary(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file)
    {
        auto err_msg = "Failed to open file: " + filename;
        throw std::runtime_error(err_msg);
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    file.close();

    return buffer;
}

VkShaderModule utils::CreateShaderModule(VulkanContext &ctx,
                                         const std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (ctx.Disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS)
    {
        std::cerr << "Failed to create a shader module!\n";
        return VK_NULL_HANDLE; // failed to create shader module
    }

    return shaderModule;
}

uint32_t utils::FindMemoryType(VulkanContext &ctx, uint32_t typeFilter,
                               VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        const auto current_flags = memProperties.memoryTypes[i].propertyFlags;

        bool memory_suitable = (typeFilter & (1 << i));
        memory_suitable &= ((current_flags & properties) == properties);

        if (memory_suitable)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void utils::CreateBuffer(VulkanContext &ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkBuffer &buffer,
                         VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.Device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(ctx, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(ctx.Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(ctx.Device, buffer, bufferMemory, 0);
}

void utils::CreateImage(VulkanContext &ctx, VkImage &image, VkDeviceMemory &imageMemory,
                        CreateImageInfo info)
{
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

    if (vkCreateImage(ctx.Device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(ctx.Device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        utils::FindMemoryType(ctx, memRequirements.memoryTypeBits, info.Properties);

    if (vkAllocateMemory(ctx.Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory!");

    vkBindImageMemory(ctx.Device, image, imageMemory, 0);
}

VkImageView utils::CreateImageView(VulkanContext &ctx, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(ctx.Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view!");

    return imageView;
}

VkCommandBuffer utils::BeginSingleTimeCommands(VulkanContext &ctx,
                                               VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void utils::EndSingleTimeCommands(VulkanContext &ctx, VkQueue queue,
                                  VkCommandPool commandPool,
                                  VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(ctx.Device, commandPool, 1, &commandBuffer);
}

void utils::CopyBuffer(VulkanContext &ctx, CopyBufferInfo info)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(ctx, info.Pool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = info.Size;
    vkCmdCopyBuffer(commandBuffer, info.Src, info.Dst, 1, &copyRegion);

    EndSingleTimeCommands(ctx, info.Queue, info.Pool, commandBuffer);
}

void utils::TransitionImageLayout(VulkanContext &ctx, TransitionImageLayoutInfo info)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(ctx, info.Pool);

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

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    EndSingleTimeCommands(ctx, info.Queue, info.Pool, commandBuffer);
}

void utils::CopyBufferToImage(VulkanContext &ctx, CopyBufferToImageInfo info)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(ctx, info.Pool);

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

    vkCmdCopyBufferToImage(commandBuffer, info.Buffer, info.Image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(ctx, info.Queue, info.Pool, commandBuffer);
}