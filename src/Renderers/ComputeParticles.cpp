#include "ComputeParticles.h"

#include "Common.h"
#include "Utils.h"

#include "Descriptor.h"
#include "Shader.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <random>
#include <vulkan/vulkan_core.h>

std::vector<VkVertexInputAttributeDescription> ComputeParticleRenderer::Vertex::
    getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
        // location, binding, format, offset
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Pos)},
    };

    return attributeDescriptions;
}

ComputeParticleRenderer::ComputeParticleRenderer(VulkanContext &ctx,
                                                 std::function<void()> callback)
    : RendererBase(ctx, callback)
{
    CreateDescriptorSets();
    CreateGraphicsPipelines();
    CreateComputePipelines();
    CreateSwapchainResources();
    CreateVertexBuffers();
    CreateUniformBuffers();
    UpdateDescriptorSets();
    CreateSyncObjects();
}

ComputeParticleRenderer::~ComputeParticleRenderer()
{
    mSwapchainDeletionQueue.flush();
    mMainDeletionQueue.flush();
}

void ComputeParticleRenderer::OnUpdate()
{
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

void ComputeParticleRenderer::OnImGui()
{
    ImGui::Begin("Compute Particles###Menu");
    callback();
    ImGui::SliderFloat("Point size", &mUBOData.PointSize, 5.0f, 100.0f);
    ImGui::SliderFloat("Speed", &mUBOData.Speed, 0.0f, 1.0f);
    ImGui::End();
}

void ComputeParticleRenderer::OnRenderImpl()
{
    auto &imageAcquiredSemaphore = mImageAcquiredSemaphores[mFrameSemaphoreIndex];
    auto &renderCompleteSemaphore = mRenderCompletedSemaphores[mFrameSemaphoreIndex];
    auto &fence = mInFlightFences[mFrameSemaphoreIndex];

    // RunCompute
    {
        auto &buffer = mComputeCommandBuffers[mFrameSemaphoreIndex];
        auto &computeFence = mComputeInFlightFences[mFrameSemaphoreIndex];

        vkWaitForFences(ctx.Device, 1, &computeFence, VK_TRUE, UINT64_MAX);

        vkResetFences(ctx.Device, 1, &computeFence);

        vkResetCommandBuffer(buffer, 0);
        RecordComputeCommandBuffer(buffer);

        auto buffers = std::array<VkCommandBuffer, 1>{buffer};

        std::array<VkSemaphore, 1> signalSemaphores{
            mComputeFinishedSemaphores[mFrameSemaphoreIndex]};

        common::SubmitQueue(mGraphicsQueue, buffers, computeFence, {}, {},
                            signalSemaphores);
    }

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

        std::array<VkSemaphore, 2> waitSemaphores{
            mComputeFinishedSemaphores[mFrameSemaphoreIndex], imageAcquiredSemaphore};

        std::array<VkPipelineStageFlags, 2> waitStages{
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        std::array<VkSemaphore, 1> signalSemaphores{renderCompleteSemaphore};

        common::SubmitQueue(mGraphicsQueue, buffers, fence, waitSemaphores, waitStages,
                            signalSemaphores);
    }

    common::PresentFrame(ctx, mPresentQueue, renderCompleteSemaphore, mFrameImageIndex);
}

void ComputeParticleRenderer::CreateSwapchainResources()
{
    CreateCommandPools();
    CreateCommandBuffers();
}

void ComputeParticleRenderer::CreateDescriptorSets()
{
    auto numFrames = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // Descriptor layout
    mDescriptorSetLayout =
        DescriptorSetLayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Build(ctx);

    // Descriptor pool
    std::vector<PoolCount> poolCounts{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numFrames},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * numFrames}};
    uint32_t maxSets = numFrames;

    mDescriptorPool = Descriptor::InitPool(ctx, maxSets, poolCounts);

    // Descriptor sets allocation
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               mDescriptorSetLayout);

    mDescriptorSets = Descriptor::Allocate(ctx, mDescriptorPool, layouts);

    mMainDeletionQueue.push_back([&]() {
        vkDestroyDescriptorPool(ctx.Device, mDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(ctx.Device, mDescriptorSetLayout, nullptr);
    });
}

void ComputeParticleRenderer::CreateGraphicsPipelines()
{
    auto shaderStages = ShaderBuilder()
                            .SetVertexPath("assets/spirv/ParticleVert.spv")
                            .SetFragmentPath("assets/spirv/ParticleFrag.spv")
                            .Build(ctx);

    auto bindingDescription =
        utils::GetBindingDescription<Vertex>(0, VK_VERTEX_INPUT_RATE_VERTEX);
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    mGraphicsPipeline = PipelineBuilder()
                            .SetShaderStages(shaderStages)
                            .SetVertexInput(bindingDescription, attributeDescriptions)
                            .SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
                            .SetPolygonMode(VK_POLYGON_MODE_FILL)
                            .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                            .DisableDepthTest()
                            .SetSwapchainColorFormat(ctx.Swapchain.image_format)
                            .EnableBlending()
                            .Build(ctx, mDescriptorSetLayout);

    mMainDeletionQueue.push_back([&]() {
        vkDestroyPipeline(ctx.Device, mGraphicsPipeline.Handle, nullptr);
        vkDestroyPipelineLayout(ctx.Device, mGraphicsPipeline.Layout, nullptr);
    });
}

void ComputeParticleRenderer::CreateComputePipelines()
{
    auto shaderStages =
        ShaderBuilder().SetComputePath("assets/spirv/ParticleComp.spv").Build(ctx);

    mComputePipeline = ComputePipelineBuilder()
                           .SetShaderStage(shaderStages[0])
                           .Build(ctx, mDescriptorSetLayout);

    mMainDeletionQueue.push_back([&]() {
        vkDestroyPipeline(ctx.Device, mComputePipeline.Handle, nullptr);
        vkDestroyPipelineLayout(ctx.Device, mComputePipeline.Layout, nullptr);
    });
}

void ComputeParticleRenderer::CreateSyncObjects()
{
    mComputeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mComputeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        utils::CreateSemaphore(ctx, mComputeFinishedSemaphores[i]);
        utils::CreateSignalledFence(ctx, mComputeInFlightFences[i]);
    }

    mMainDeletionQueue.push_back([&]() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(ctx.Device, mComputeFinishedSemaphores[i], nullptr);
            vkDestroyFence(ctx.Device, mComputeInFlightFences[i], nullptr);
        }
    });
}

void ComputeParticleRenderer::CreateCommandPools()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(ctx.Device, &pool_info, nullptr, &mCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");

    mSwapchainDeletionQueue.push_back(
        [&]() { vkDestroyCommandPool(ctx.Device, mCommandPool, nullptr); });
}

void ComputeParticleRenderer::CreateCommandBuffers()
{
    mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    mComputeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, mCommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, mComputeCommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void ComputeParticleRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
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

        std::array<VkBuffer, 1> vertexBuffers{
            mVertexBuffers[mFrameSemaphoreIndex].Handle};
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

void ComputeParticleRenderer::RecordComputeCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      mComputePipeline.Handle);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            mComputePipeline.Layout, 0, 1,
                            &mDescriptorSets[mFrameSemaphoreIndex], 0, 0);

    vkCmdDispatch(commandBuffer, mVertexCount / 256, 1, 1);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void ComputeParticleRenderer::CreateVertexBuffers()
{
    constexpr size_t particleCount = 512;

    std::vector<Vertex> vertices(particleCount);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    for (size_t i = 0; i < particleCount; i++)
    {
        vertices[i].Pos.x = dis(gen);
        vertices[i].Pos.y = dis(gen);

        float theta = 3.1415f * dis(gen);
        vertices[i].Velocity.x = 0.01f * std::cos(theta);
        vertices[i].Velocity.y = 0.01f * std::sin(theta);
    }

    mVertexCount = vertices.size();

    mVertexBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto &buffer : mVertexBuffers)
    {
        GPUBufferInfo info{
            .Queue = mGraphicsQueue,
            .Pool = mCommandPool,
            .Data = vertices.data(),
            .Size = mVertexCount * sizeof(Vertex),
            .Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        buffer = Buffer::CreateGPUBuffer(ctx, info);
    }

    mMainDeletionQueue.push_back([&]() {
        for (auto &buffer : mVertexBuffers)
            Buffer::DestroyBuffer(ctx, buffer);
    });
}

void ComputeParticleRenderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    mUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto &uniformBuffer : mUniformBuffers)
        uniformBuffer.OnInit(ctx, bufferSize);

    mMainDeletionQueue.push_back([&]() {
        for (auto &uniformBuffer : mUniformBuffers)
            uniformBuffer.OnDestroy(ctx);
    });
}

void ComputeParticleRenderer::UpdateDescriptorSets()
{
    for (size_t i = 0; i < mDescriptorSets.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mUniformBuffers[i].Handle();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = mDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        storageBufferInfoLastFrame.buffer =
            mVertexBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT].Handle;
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = sizeof(Vertex) * mVertexCount;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = mDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = mVertexBuffers[i].Handle;
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = sizeof(Vertex) * mVertexCount;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = mDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

        vkUpdateDescriptorSets(ctx.Device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}