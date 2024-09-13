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

static uint32_t FindMemoryType(VulkanContext &ctx, uint32_t typeFilter,
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

void utils::CopyBuffer(VulkanContext &ctx, CopyBufferInfo info)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = info.Pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = info.Size;
    vkCmdCopyBuffer(commandBuffer, info.Src, info.Dst, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(info.Queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(info.Queue);

    vkFreeCommandBuffers(ctx.Device, info.Pool, 1, &commandBuffer);
}