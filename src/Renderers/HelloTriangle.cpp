#include "HelloTriangle.h"

#include "Common.h"
#include "Descriptor.h"
#include "Shader.h"
#include "Utils.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <array>

#include <glm/gtc/matrix_transform.hpp>

std::vector<VkVertexInputAttributeDescription> HelloTriangleRenderer::Vertex::
    getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
        // location, binding, format, offset
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)},
    };

    return attributeDescriptions;
}

HelloTriangleRenderer::HelloTriangleRenderer(VulkanContext &ctx,
                                             std::function<void()> callback)
    : RendererBase(ctx, callback)
{
    CreateDescriptorSets();
    CreateGraphicsPipelines();
    CreateSwapchainResources();
    CreateVertexBuffers();
    CreateUniformBuffers();
    UpdateDescriptorSets();
}

HelloTriangleRenderer::~HelloTriangleRenderer()
{
    mSwapchainDeletionQueue.flush();
    mMainDeletionQueue.flush();
}

void HelloTriangleRenderer::OnUpdate()
{
    // Update Uniform buffer data:
    auto width = static_cast<float>(ctx.Swapchain.extent.width);
    auto height = static_cast<float>(ctx.Swapchain.extent.height);

    float sx = 1.0f, sy = 1.0f;

    if (height < width)
        sx = width / height;
    else
        sy = height / width;

    auto proj = glm::ortho(-sx, sx, -sy, sy, -1.0f, 1.0f);

    mUBOData.MVP = proj;

    auto &uniformBuffer = mUniformBuffers[mFrameSemaphoreIndex];
    uniformBuffer.UploadData(&mUBOData, sizeof(mUBOData));
}

void HelloTriangleRenderer::OnImGui()
{
    ImGui::Begin("Hello Triangle");
    callback();
    ImGui::SliderFloat("Rotation", &mUBOData.Phi, 0.0f, 6.28f);
    ImGui::End();
}

void HelloTriangleRenderer::OnRenderImpl()
{
    auto &imageAcquiredSemaphore = mImageAcquiredSemaphores[mFrameSemaphoreIndex];
    auto &renderCompleteSemaphore = mRenderCompletedSemaphores[mFrameSemaphoreIndex];
    auto &fence = mInFlightFences[mFrameSemaphoreIndex];

    vkWaitForFences(ctx.Device, 1, &fence, VK_TRUE, UINT64_MAX);

    common::AcquireNextImage(ctx, imageAcquiredSemaphore, mFrameImageIndex);

    if (!ctx.SwapchainOk)
        return;

    vkResetFences(ctx.Device, 1, &fence);

    // DrawFrame
    {
        auto &buffer = mCommandBuffers[mFrameSemaphoreIndex];

        vkResetCommandBuffer(buffer, 0);
        RecordCommandBuffer(buffer, mFrameImageIndex);

        auto buffers = std::array<VkCommandBuffer, 1>{buffer};

        common::SubmitGraphicsQueueDefault(mGraphicsQueue, buffers, fence,
                                           imageAcquiredSemaphore,
                                           renderCompleteSemaphore);
    }

    common::PresentFrame(ctx, mPresentQueue, renderCompleteSemaphore, mFrameImageIndex);
}

void HelloTriangleRenderer::CreateSwapchainResources()
{
    CreateCommandPools();
    CreateCommandBuffers();
}

void HelloTriangleRenderer::CreateDescriptorSets()
{
    auto numFrames = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // Descriptor layout
    mDescriptorSetLayout =
        DescriptorSetLayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .Build(ctx);

    // Descriptor pool
    std::vector<PoolCount> poolCounts{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numFrames},
    };
    uint32_t maxSets = numFrames;

    mDescriptorPool = Descriptor::InitPool(ctx, maxSets, poolCounts);

    // Descriptor sets allocation
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               mDescriptorSetLayout);

    mDescriptorSets = Descriptor::Allocate(ctx, mDescriptorPool, layouts);

    mMainDeletionQueue.push_back([&](){
        vkDestroyDescriptorPool(ctx.Device, mDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(ctx.Device, mDescriptorSetLayout, nullptr);
    });
}

void HelloTriangleRenderer::CreateGraphicsPipelines()
{
    auto shaderStages = ShaderBuilder()
                            .SetVertexPath("assets/spirv/HelloTriangleVert.spv")
                            .SetFragmentPath("assets/spirv/HelloTriangleFrag.spv")
                            .Build(ctx);

    auto bindingDescription =
        utils::GetBindingDescription<Vertex>(0, VK_VERTEX_INPUT_RATE_VERTEX);
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    mGraphicsPipeline = PipelineBuilder()
                            .SetShaderStages(shaderStages)
                            .SetVertexInput(bindingDescription, attributeDescriptions)
                            .SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                            .SetPolygonMode(VK_POLYGON_MODE_FILL)
                            .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                            .DisableDepthTest()
                            .SetSwapchainColorFormat(ctx.Swapchain.image_format)
                            .Build(ctx, mDescriptorSetLayout);

    mMainDeletionQueue.push_back([&](){
        vkDestroyPipeline(ctx.Device, mGraphicsPipeline.Handle, nullptr);
        vkDestroyPipelineLayout(ctx.Device, mGraphicsPipeline.Layout, nullptr);
    });
}

void HelloTriangleRenderer::CreateCommandPools()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(ctx.Device, &pool_info, nullptr, &mCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");

    mSwapchainDeletionQueue.push_back([&](){
        vkDestroyCommandPool(ctx.Device, mCommandPool, nullptr);
    });
}

void HelloTriangleRenderer::CreateCommandBuffers()
{
    mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, mCommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void HelloTriangleRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                                uint32_t imageIndex)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    common::ImageBarrierColorToRender(commandBuffer, ctx.SwapchainImages[imageIndex]);

    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = ctx.SwapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = {
        {0, 0}, {ctx.Swapchain.extent.width, ctx.Swapchain.extent.height}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mGraphicsPipeline.Handle);

        common::ViewportScissorDefaultBehaviour(ctx, commandBuffer);

        std::array<VkBuffer, 1> vertexBuffers{mVertexBuffer.Handle};
        std::array<VkDeviceSize, 1> offsets{0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mGraphicsPipeline.Layout, 0, 1,
                                &mDescriptorSets[mFrameSemaphoreIndex], 0, nullptr);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(mVertexCount), 1, 0, 0);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }
    vkCmdEndRendering(commandBuffer);

    common::ImageBarrierColorToPresent(commandBuffer, ctx.SwapchainImages[imageIndex]);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void HelloTriangleRenderer::CreateVertexBuffers()
{
    // clang-format off
    const float r3 = std::sqrt(3.0f);

    const std::vector<Vertex> vertices{
        {{ 0.0f,-r3/3.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, r3/6.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, r3/6.0f}, {0.0f, 0.0f, 1.0f}}
    };
    // clang-format on

    mVertexCount = vertices.size();

    GPUBufferInfo info{
        .Queue = mGraphicsQueue,
        .Pool = mCommandPool,
        .Data = vertices.data(),
        .Size = mVertexCount * sizeof(Vertex),
        .Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    mVertexBuffer = Buffer::CreateGPUBuffer(ctx, info);

    mMainDeletionQueue.push_back([&](){
        Buffer::DestroyBuffer(ctx, mVertexBuffer);
    });
}

void HelloTriangleRenderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    mUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto &uniformBuffer : mUniformBuffers)
        uniformBuffer.OnInit(ctx, bufferSize);

    mMainDeletionQueue.push_back([&](){
        for (auto &uniformBuffer : mUniformBuffers)
            uniformBuffer.OnDestroy(ctx);
    });
}

void HelloTriangleRenderer::UpdateDescriptorSets()
{
    for (size_t i = 0; i < mDescriptorSets.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mUniformBuffers[i].Handle();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = mDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(ctx.Device, 1, &descriptorWrite, 0, nullptr);
    }
}