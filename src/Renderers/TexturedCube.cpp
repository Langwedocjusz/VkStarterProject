#include "TexturedCube.h"

#include "Common.h"
#include "Utils.h"

#include "Descriptor.h"
#include "ImageLoaders.h"
#include "ImageView.h"
#include "Sampler.h"
#include "Shader.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>

std::vector<VkVertexInputAttributeDescription> TexturedCubeRenderer::Vertex::
    getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
        // location, binding, format, offset
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)},
    };

    return attributeDescriptions;
}

TexturedCubeRenderer::TexturedCubeRenderer(VulkanContext &ctx,
                                           std::function<void()> callback)
    : RendererBase(ctx, callback)
{
    CreateDescriptorSets();
    CreateGraphicsPipelines();
    CreateSwapchainResources();
    CreateTextureResources();
    CreateVertexBuffers();
    CreateIndexBuffers();
    CreateUniformBuffers();
    UpdateDescriptorSets();
}

TexturedCubeRenderer::~TexturedCubeRenderer()
{
    mSwapchainDeletionQueue.flush();
    mMainDeletionQueue.flush();
}

void TexturedCubeRenderer::OnUpdate()
{
    // Update Uniform buffer data:
    auto width = static_cast<float>(ctx.Swapchain.extent.width);
    auto height = static_cast<float>(ctx.Swapchain.extent.height);

    float aspect = width / height;

    glm::vec3 pos{0.0f, 0.0f, -3.0f};
    glm::vec3 front{0.0f, 0.0f, 1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    auto proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);
    auto view = glm::lookAt(pos, pos + front, up);

    mUBOData.MVP = proj * view;

    auto &uniformBuffer = mUniformBuffers[mFrameSemaphoreIndex];
    uniformBuffer.UploadData(&mUBOData, sizeof(mUBOData));
}

void TexturedCubeRenderer::OnImGui()
{
    ImGui::Begin("Textured Cube");
    callback();
    ImGui::SliderFloat("Rotation", &mUBOData.Phi, 0.0f, 6.28f);
    ImGui::End();
}

void TexturedCubeRenderer::OnRenderImpl()
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

void TexturedCubeRenderer::CreateSwapchainResources()
{
    CreateDepthResources();
    CreateCommandPools();
    CreateCommandBuffers();
}

void TexturedCubeRenderer::CreateDescriptorSets()
{
    auto numFrames = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // Descriptor layout
    mDescriptorSetLayout =
        DescriptorSetLayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT)
            .Build(ctx);

    // Descriptor pool
    std::vector<PoolCount> poolCounts{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numFrames},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numFrames},
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

void TexturedCubeRenderer::CreateGraphicsPipelines()
{
    auto shaderStages = ShaderBuilder()
                            .SetVertexPath("assets/spirv/TexturedCubeVert.spv")
                            .SetFragmentPath("assets/spirv/TexturedCubeFrag.spv")
                            .Build(ctx);

    auto bindingDescription =
        utils::GetBindingDescription<Vertex>(0, VK_VERTEX_INPUT_RATE_VERTEX);
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkFormat depthFormat = utils::FindDepthFormat(ctx);

    mGraphicsPipeline = PipelineBuilder()
                            .SetShaderStages(shaderStages)
                            .SetVertexInput(bindingDescription, attributeDescriptions)
                            .SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                            .SetPolygonMode(VK_POLYGON_MODE_FILL)
                            .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                            .EnableDepthTest()
                            .SetSwapchainColorFormat(ctx.Swapchain.image_format)
                            .SetDepthFormat(depthFormat)
                            .Build(ctx, mDescriptorSetLayout);

    mMainDeletionQueue.push_back([&](){
        vkDestroyPipeline(ctx.Device, mGraphicsPipeline.Handle, nullptr);
        vkDestroyPipelineLayout(ctx.Device, mGraphicsPipeline.Layout, nullptr);
    });
}

void TexturedCubeRenderer::CreateCommandPools()
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

void TexturedCubeRenderer::CreateCommandBuffers()
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

void TexturedCubeRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                               uint32_t imageIndex)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    common::ImageBarrierColorToRender(commandBuffer, ctx.SwapchainImages[imageIndex]);
    common::ImageBarrierDepthToRender(commandBuffer, mDepthImage.Handle);

    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = ctx.SwapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    VkRenderingAttachmentInfoKHR depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depthAttachment.imageView = mDepthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = {
        {0, 0}, {ctx.Swapchain.extent.width, ctx.Swapchain.extent.height}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;
    // renderingInfo.pStencilAttachment = &stencilAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mGraphicsPipeline.Handle);

        common::ViewportScissorDefaultBehaviour(ctx, commandBuffer);

        std::array<VkBuffer, 1> vertexBuffers{mVertexBuffer.Handle};
        std::array<VkDeviceSize, 1> offsets{0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());

        vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer.Handle, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mGraphicsPipeline.Layout, 0, 1,
                                &mDescriptorSets[mFrameSemaphoreIndex], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mIndexCount), 1, 0, 0, 0);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }

    vkCmdEndRendering(commandBuffer);

    common::ImageBarrierColorToPresent(commandBuffer, ctx.SwapchainImages[imageIndex]);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void TexturedCubeRenderer::CreateVertexBuffers()
{
    // clang-format off
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f},  {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f},  {0.0f, 1.0f}},

        {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},

        {{0.5f,  0.5f,  0.5f},   {1.0f, 0.0f}},
        {{0.5f,  0.5f, -0.5f},   {1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f},   {0.0f, 1.0f}},
        {{0.5f, -0.5f,  0.5f},   {0.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f},  {1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},

        {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f},  {0.0f, 0.0f}},
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

void TexturedCubeRenderer::CreateIndexBuffers()
{
    // clang-format off
    const std::vector<uint16_t> indices = {
        0, 2, 1, 2, 0, 3,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 14, 13, 14, 12, 15,
        16, 17, 18, 18, 19, 16,
        20, 22, 21, 22, 20, 23
    };
    // clang-format on

    mIndexCount = indices.size();

    GPUBufferInfo info{
        .Queue = mGraphicsQueue,
        .Pool = mCommandPool,
        .Data = indices.data(),
        .Size = mIndexCount * sizeof(uint16_t),
        .Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    mIndexBuffer = Buffer::CreateGPUBuffer(ctx, info);

    mMainDeletionQueue.push_back([&](){
        Buffer::DestroyBuffer(ctx, mIndexBuffer);
    });
}

void TexturedCubeRenderer::CreateUniformBuffers()
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

void TexturedCubeRenderer::UpdateDescriptorSets()
{
    for (size_t i = 0; i < mDescriptorSets.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mUniformBuffers[i].Handle();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = mTextureImageView;
        imageInfo.sampler = mTextureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = mDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = mDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(ctx.Device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void TexturedCubeRenderer::CreateTextureResources()
{
    ImageLoaderInfo info{
        .Queue = mGraphicsQueue,
        .Pool = mCommandPool,
        .Filepath = "assets/textures/container.jpg",
    };

    mTextureImage = ImageLoaders::LoadImage2D(ctx, info);

    mTextureImageView =
        ImageView::Create(ctx, mTextureImage.Handle, VK_FORMAT_R8G8B8A8_SRGB);

    mTextureSampler = SamplerBuilder()
                          .SetMagFilter(VK_FILTER_LINEAR)
                          .SetMinFilter(VK_FILTER_LINEAR)
                          .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
                          .Build(ctx);

    mMainDeletionQueue.push_back([&](){
        vkDestroySampler(ctx.Device, mTextureSampler, nullptr);
        vkDestroyImageView(ctx.Device, mTextureImageView, nullptr);
        Image::DestroyImage(ctx, mTextureImage);
    });
}

void TexturedCubeRenderer::CreateDepthResources()
{
    VkFormat depthFormat = utils::FindDepthFormat(ctx);

    ImageInfo info{
        .Width = ctx.Swapchain.extent.width,
        .Height = ctx.Swapchain.extent.height,
        .Format = depthFormat,
        .Tiling = VK_IMAGE_TILING_OPTIMAL,
        .Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    mDepthImage = Image::CreateImage(ctx, info);

    mDepthImageView = ImageView::Create(ctx, mDepthImage.Handle, depthFormat,
                                        VK_IMAGE_ASPECT_DEPTH_BIT);

    mSwapchainDeletionQueue.push_back([&](){
        vkDestroyImageView(ctx.Device, mDepthImageView, nullptr);
        Image::DestroyImage(ctx, mDepthImage);
    });
}