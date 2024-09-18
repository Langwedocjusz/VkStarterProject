#include "Buffer.h"

#include "Utils.h"

uint32_t Buffer::FindMemoryType(VulkanContext &ctx, uint32_t typeFilter,
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

Buffer Buffer::CreateBuffer(VulkanContext &ctx, VkDeviceSize size,
                            VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    Buffer buf;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.Device, &bufferInfo, nullptr, &buf.Handle) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.Device, buf.Handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(ctx, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(ctx.Device, &allocInfo, nullptr, &buf.Memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory!");

    vkBindBufferMemory(ctx.Device, buf.Handle, buf.Memory, 0);

    return buf;
}

void Buffer::DestroyBuffer(VulkanContext &ctx, Buffer &buf)
{
    vkDestroyBuffer(ctx.Device, buf.Handle, nullptr);
    vkFreeMemory(ctx.Device, buf.Memory, nullptr);
}

void Buffer::UploadToBuffer(VulkanContext &ctx, Buffer buff, const void *data,
                            VkDeviceSize size)
{
    void *dst;
    vkMapMemory(ctx.Device, buff.Memory, 0, size, 0, &dst);
    std::memcpy(dst, data, static_cast<size_t>(size));
    vkUnmapMemory(ctx.Device, buff.Memory);
}

Buffer Buffer::CreateStagingBuffer(VulkanContext &ctx, VkDeviceSize size)
{
    auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return CreateBuffer(ctx, size, usage, properties);
}

Buffer Buffer::CreateMappedUniformBuffer(VulkanContext &ctx, VkDeviceSize size)
{
    auto usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return CreateBuffer(ctx, size, usage, properties);
}

Buffer Buffer::CreateGPUBuffer(VulkanContext &ctx, GPUBufferInfo info)
{
    Buffer buff;
    buff = CreateBuffer(ctx, info.Size, info.Usage, info.Properties);

    Buffer stagingBuffer = Buffer::CreateStagingBuffer(ctx, info.Size);
    UploadToBuffer(ctx, stagingBuffer, info.Data, info.Size);

    utils::CopyBufferInfo cp_info{
        .Queue = info.Queue,
        .Pool = info.Pool,
        .Src = stagingBuffer.Handle,
        .Dst = buff.Handle,
        .Size = info.Size,
    };

    utils::CopyBuffer(ctx, cp_info);

    DestroyBuffer(ctx, stagingBuffer);

    return buff;
}


void MappedUniformBuffer::OnInit(VulkanContext &ctx, VkDeviceSize size)
{
    mBuffer = Buffer::CreateMappedUniformBuffer(ctx, size);
    vkMapMemory(ctx.Device, mBuffer.Memory, 0, size, 0, &mData);
}

void MappedUniformBuffer::OnDestroy(VulkanContext &ctx)
{
    Buffer::DestroyBuffer(ctx, mBuffer);
}

void MappedUniformBuffer::UploadData(const void* data, size_t size)
{
    std::memcpy(mData, data, size);
}