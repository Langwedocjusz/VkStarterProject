#include "TexturedCube.h"

#include "Utils.h"

#include "ImageLoaders.h"
#include "Shader.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <iostream>

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

void TexturedCubeRenderer::OnImGui()
{
    ImGui::Begin("Textured Cube");
    callback();
    ImGui::SliderFloat("Rotation", &UBOData.Phi, 0.0f, 6.28f);
    ImGui::End();
}

void TexturedCubeRenderer::CreateResources()
{
    CreateDescriptorSetLayout();
    CreateGraphicsPipelines();
}

void TexturedCubeRenderer::CreateSwapchainResources()
{
    CreateDepthResources();
    CreateCommandPools();
    CreateCommandBuffers();
}

void TexturedCubeRenderer::CreateDependentResources()
{
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
    CreateVertexBuffers();
    CreateIndexBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
}

void TexturedCubeRenderer::DestroyResources()
{
    vkDestroySampler(ctx.Device, TextureSampler, nullptr);
    vkDestroyImageView(ctx.Device, TextureImageView, nullptr);
    Image::DestroyImage(ctx, TextureImage);

    Buffer::DestroyBuffer(ctx, VertexBuffer);
    Buffer::DestroyBuffer(ctx, IndexBuffer);

    vkDestroyPipeline(ctx.Device, GraphicsPipeline.Handle, nullptr);
    vkDestroyPipelineLayout(ctx.Device, GraphicsPipeline.Layout, nullptr);

    for (auto &uniformBuffer : UniformBuffers)
        uniformBuffer.OnDestroy(ctx);

    vkDestroyDescriptorPool(ctx.Device, DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(ctx.Device, DescriptorSetLayout, nullptr);
}

void TexturedCubeRenderer::DestroySwapchainResources()
{
    vkDestroyCommandPool(ctx.Device, CommandPool, nullptr);

    vkDestroyImageView(ctx.Device, DepthImageView, nullptr);
    Image::DestroyImage(ctx, DepthImage);
}

void TexturedCubeRenderer::CreateDescriptorSetLayout()
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
                                    &DescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
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

    GraphicsPipeline = PipelineBuilder()
                           .SetShaderStages(shaderStages)
                           .SetVertexInput(bindingDescription, attributeDescriptions)
                           .SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                           .SetPolygonMode(VK_POLYGON_MODE_FILL)
                           .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                           .EnableDepthTest()
                           .SetSwapchainColorFormat(ctx.Swapchain.image_format)
                           .SetDepthFormat(depthFormat)
                           .Build(ctx, DescriptorSetLayout);
}

void TexturedCubeRenderer::CreateCommandPools()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(ctx.Device, &pool_info, nullptr, &CommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");
}

void TexturedCubeRenderer::CreateCommandBuffers()
{
    CommandBuffers.resize(SwapchainImageViews.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)CommandBuffers.size();

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, CommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void TexturedCubeRenderer::SubmitCommandBuffers()
{
    auto &imageAcquiredSemaphore = ImageAcquiredSemaphores[FrameSemaphoreIndex];
    auto &renderCompleteSemaphore = RenderCompletedSemaphores[FrameSemaphoreIndex];

    vkResetCommandBuffer(CommandBuffers[FrameSemaphoreIndex], 0);
    RecordCommandBuffer(CommandBuffers[FrameSemaphoreIndex], FrameImageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {imageAcquiredSemaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &CommandBuffers[FrameSemaphoreIndex];

    VkSemaphore signal_semaphores[] = {renderCompleteSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo,
                      InFlightFences[FrameSemaphoreIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
}

void TexturedCubeRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                               uint32_t imageIndex)
{
    UpdateUniformBuffer();

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    utils::ImageBarrierColorToRender(commandBuffer, SwapchainImages[imageIndex]);
    utils::ImageBarrierDepthToRender(commandBuffer, DepthImage.Handle);

    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = SwapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingAttachmentInfoKHR depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depthAttachment.imageView = DepthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = {0, 0, ctx.Swapchain.extent.width,
                                ctx.Swapchain.extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;
    // renderingInfo.pStencilAttachment = &stencilAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          GraphicsPipeline.Handle);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(ctx.Swapchain.extent.width);
        viewport.height = static_cast<float>(ctx.Swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = ctx.Swapchain.extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {VertexBuffer.Handle};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, IndexBuffer.Handle, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                GraphicsPipeline.Layout, 0, 1,
                                &DescriptorSets[FrameSemaphoreIndex], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(IndexCount), 1, 0, 0, 0);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }

    vkCmdEndRendering(commandBuffer);

    utils::ImageBarrierColorToPresent(commandBuffer, SwapchainImages[imageIndex]);

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

    VertexCount = vertices.size();

    GPUBufferInfo info{
        .Queue = GraphicsQueue,
        .Pool = CommandPool,
        .Data = vertices.data(),
        .Size = VertexCount * sizeof(Vertex),
        .Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VertexBuffer = Buffer::CreateGPUBuffer(ctx, info);
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

    IndexCount = indices.size();

    GPUBufferInfo info{
        .Queue = GraphicsQueue,
        .Pool = CommandPool,
        .Data = indices.data(),
        .Size = IndexCount * sizeof(uint16_t),
        .Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    IndexBuffer = Buffer::CreateGPUBuffer(ctx, info);
}

void TexturedCubeRenderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto &uniformBuffer : UniformBuffers)
        uniformBuffer.OnInit(ctx, bufferSize);
}

void TexturedCubeRenderer::UpdateUniformBuffer()
{
    auto width = static_cast<float>(ctx.Swapchain.extent.width);
    auto height = static_cast<float>(ctx.Swapchain.extent.height);

    float aspect = width / height;

    glm::vec3 pos{0.0f, 0.0f, -3.0f};
    glm::vec3 front{0.0f, 0.0f, 1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    auto proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);
    auto view = glm::lookAt(pos, pos + front, up);

    UBOData.MVP = proj * view;

    auto &uniformBuffer = UniformBuffers[FrameSemaphoreIndex];
    uniformBuffer.UploadData(&UBOData, sizeof(UBOData));
}

void TexturedCubeRenderer::CreateDescriptorPool()
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

    if (vkCreateDescriptorPool(ctx.Device, &poolInfo, nullptr, &DescriptorPool) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void TexturedCubeRenderer::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(ctx.Device, &allocInfo, DescriptorSets.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = UniformBuffers[i].Handle();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = TextureImageView;
        imageInfo.sampler = TextureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = DescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = DescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(ctx.Device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void TexturedCubeRenderer::CreateTextureImage()
{
    ImageLoaderInfo info{
        .Queue = GraphicsQueue,
        .Pool = CommandPool,
        .Filepath = "assets/textures/container.jpg",
    };

    TextureImage = ImageLoaders::LoadImage2D(ctx, info);
}

void TexturedCubeRenderer::CreateTextureImageView()
{
    TextureImageView =
        utils::CreateImageView(ctx, TextureImage.Handle, VK_FORMAT_R8G8B8A8_SRGB);
}

void TexturedCubeRenderer::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(ctx.PhysicalDevice, &properties);

    // Overkill, but why not
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(ctx.Device, &samplerInfo, nullptr, &TextureSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}

void TexturedCubeRenderer::CreateDepthResources()
{
    VkFormat depthFormat = utils::FindDepthFormat(ctx);

    {
        ImageInfo info{
            .Width = ctx.Swapchain.extent.width,
            .Height = ctx.Swapchain.extent.height,
            .Format = depthFormat,
            .Tiling = VK_IMAGE_TILING_OPTIMAL,
            .Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        DepthImage = Image::CreateImage(ctx, info);
    }

    DepthImageView = utils::CreateImageView(ctx, DepthImage.Handle, depthFormat,
                                            VK_IMAGE_ASPECT_DEPTH_BIT);
}