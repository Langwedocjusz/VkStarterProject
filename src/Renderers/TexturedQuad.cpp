#include "TexturedQuad.h"

#include "Utils.h"

#include "ImageLoaders.h"
#include "ImageView.h"
#include "Sampler.h"
#include "Shader.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>

std::vector<VkVertexInputAttributeDescription> TexturedQuadRenderer::Vertex::
    getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
        // location, binding, format, offset
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)},
    };

    return attributeDescriptions;
}

TexturedQuadRenderer::TexturedQuadRenderer(VulkanContext &ctx,
                                           std::function<void()> callback)
    : RendererBase(ctx, callback)
{
    CreateDescriptorSetLayout();
    CreateGraphicsPipelines();
    CreateSwapchainResources();
    CreateTextureResources();
    CreateVertexBuffers();
    CreateIndexBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
}

TexturedQuadRenderer::~TexturedQuadRenderer()
{
    DestroySwapchainResources();

    vkDestroySampler(ctx.Device, mTextureSampler, nullptr);
    vkDestroyImageView(ctx.Device, mTextureImageView, nullptr);
    Image::DestroyImage(ctx, mTextureImage);

    Buffer::DestroyBuffer(ctx, mVertexBuffer);
    Buffer::DestroyBuffer(ctx, mIndexBuffer);

    vkDestroyPipeline(ctx.Device, mGraphicsPipeline.Handle, nullptr);
    vkDestroyPipelineLayout(ctx.Device, mGraphicsPipeline.Layout, nullptr);

    for (auto &uniformBuffer : mUniformBuffers)
        uniformBuffer.OnDestroy(ctx);

    vkDestroyDescriptorPool(ctx.Device, mDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(ctx.Device, mDescriptorSetLayout, nullptr);
}

void TexturedQuadRenderer::OnImGui()
{
    ImGui::Begin("Textured Quad");
    callback();
    ImGui::SliderFloat("Rotation", &mUBOData.Phi, 0.0f, 6.28f);
    ImGui::End();
}

void TexturedQuadRenderer::CreateSwapchainResources()
{
    CreateCommandPools();
    CreateCommandBuffers();
}

void TexturedQuadRenderer::DestroySwapchainResources()
{
    vkDestroyCommandPool(ctx.Device, mCommandPool, nullptr);
}

void TexturedQuadRenderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                            samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(ctx.Device, &layoutInfo, nullptr,
                                    &mDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
}

void TexturedQuadRenderer::CreateGraphicsPipelines()
{
    auto shaderStages = ShaderBuilder()
                            .SetVertexPath("assets/spirv/TexturedQuadVert.spv")
                            .SetFragmentPath("assets/spirv/TexturedQuadFrag.spv")
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
}

void TexturedQuadRenderer::CreateCommandPools()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(ctx.Device, &pool_info, nullptr, &mCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");
}

void TexturedQuadRenderer::CreateCommandBuffers()
{
    mCommandBuffers.resize(mSwapchainImageViews.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, mCommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void TexturedQuadRenderer::SubmitCommandBuffers()
{
    auto &imageAcquiredSemaphore = mImageAcquiredSemaphores[mFrameSemaphoreIndex];
    auto &renderCompleteSemaphore = mRenderCompletedSemaphores[mFrameSemaphoreIndex];

    vkResetCommandBuffer(mCommandBuffers[mFrameSemaphoreIndex], 0);
    RecordCommandBuffer(mCommandBuffers[mFrameSemaphoreIndex], mFrameImageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {imageAcquiredSemaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[mFrameSemaphoreIndex];

    VkSemaphore signal_semaphores[] = {renderCompleteSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo,
                      mInFlightFences[mFrameSemaphoreIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
}

void TexturedQuadRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                               uint32_t imageIndex)
{
    UpdateUniformBuffer();

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    utils::ImageBarrierColorToRender(commandBuffer, mSwapchainImages[imageIndex]);

    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = mSwapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = {0, 0, ctx.Swapchain.extent.width,
                                ctx.Swapchain.extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mGraphicsPipeline.Handle);

        utils::ViewportScissorDefaultBehaviour(ctx, commandBuffer);

        VkBuffer vertexBuffers[] = {mVertexBuffer.Handle};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer.Handle, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mGraphicsPipeline.Layout, 0, 1,
                                &mDescriptorSets[mFrameSemaphoreIndex], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mIndexCount), 1, 0, 0, 0);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }
    vkCmdEndRendering(commandBuffer);

    utils::ImageBarrierColorToPresent(commandBuffer, mSwapchainImages[imageIndex]);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void TexturedQuadRenderer::CreateVertexBuffers()
{
    // clang-format off
    const std::vector<Vertex> vertices = {
        {{-0.5f,-0.5f}, {1.0f, 0.0f}},
        {{ 0.5f,-0.5f}, {0.0f, 0.0f}},
        {{ 0.5f, 0.5f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f}}
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
}

void TexturedQuadRenderer::CreateIndexBuffers()
{
    // clang-format off
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
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
}

void TexturedQuadRenderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    mUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto &uniformBuffer : mUniformBuffers)
        uniformBuffer.OnInit(ctx, bufferSize);
}

void TexturedQuadRenderer::UpdateUniformBuffer()
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

void TexturedQuadRenderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(ctx.Device, &poolInfo, nullptr, &mDescriptorPool) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void TexturedQuadRenderer::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               mDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = mDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    mDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(ctx.Device, &allocInfo, mDescriptorSets.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
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

void TexturedQuadRenderer::CreateTextureResources()
{
    ImageLoaderInfo info{
        .Queue = mGraphicsQueue,
        .Pool = mCommandPool,
        .Filepath = "assets/textures/texture.jpg",
    };

    mTextureImage = ImageLoaders::LoadImage2D(ctx, info);

    mTextureImageView =
        ImageView::Create(ctx, mTextureImage.Handle, VK_FORMAT_R8G8B8A8_SRGB);

    mTextureSampler = SamplerBuilder()
                          .SetMagFilter(VK_FILTER_LINEAR)
                          .SetMinFilter(VK_FILTER_LINEAR)
                          .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
                          .Build(ctx);
}