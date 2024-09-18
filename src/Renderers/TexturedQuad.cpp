#include "TexturedQuad.h"

#include "Utils.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>

VkVertexInputBindingDescription TexturedQuadRenderer::Vertex::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> TexturedQuadRenderer::Vertex::
    getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{{
        // location, binding, format, offset
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)},
    }};

    return attributeDescriptions;
}

void TexturedQuadRenderer::OnImGui()
{
    ImGui::Begin("Textured Quad");
    callback();
    ImGui::SliderFloat("Rotation", &UBOData.Phi, 0.0f, 6.28f);
    ImGui::End();
}

void TexturedQuadRenderer::CreateResources()
{
    CreateRenderPasses();
    CreateDescriptorSetLayout();
    CreateGraphicsPipelines();
}

void TexturedQuadRenderer::CreateSwapchainResources()
{
    CreateFramebuffers();
    CreateCommandPools();
    CreateCommandBuffers();
}

void TexturedQuadRenderer::CreateDependentResources()
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

void TexturedQuadRenderer::DestroyResources()
{
    vkDestroySampler(ctx.Device, TextureSampler, nullptr);
    vkDestroyImageView(ctx.Device, TextureImageView, nullptr);
    Image::DestroyImage(ctx, TextureImage);

    Buffer::DestroyBuffer(ctx, VertexBuffer);
    Buffer::DestroyBuffer(ctx, IndexBuffer);

    vkDestroyPipeline(ctx.Device, GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(ctx.Device, PipelineLayout, nullptr);
    vkDestroyRenderPass(ctx.Device, RenderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        Buffer::DestroyBuffer(ctx, UniformBuffers[i]);

    vkDestroyDescriptorPool(ctx.Device, DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(ctx.Device, DescriptorSetLayout, nullptr);
}

void TexturedQuadRenderer::DestroySwapchainResources()
{
    vkDestroyCommandPool(ctx.Device, CommandPool, nullptr);

    for (auto framebuffer : Framebuffers)
    {
        vkDestroyFramebuffer(ctx.Device, framebuffer, nullptr);
    }
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
                                    &DescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
}

void TexturedQuadRenderer::CreateRenderPasses()
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = ctx.Swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(ctx.Device, &render_pass_info, nullptr, &RenderPass) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create a render pass!");
}

void TexturedQuadRenderer::CreateGraphicsPipelines()
{
    // Graphics pipeline:
    auto vert_code = utils::ReadFileBinary("assets/spirv/TexturedQuadVert.spv");
    auto frag_code = utils::ReadFileBinary("assets/spirv/TexturedQuadFrag.spv");

    VkShaderModule vert_module = utils::CreateShaderModule(ctx, vert_code);
    VkShaderModule frag_module = utils::CreateShaderModule(ctx, frag_code);

    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to create a shader module!");

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

    // Vertex Input setup:
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
    vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(ctx.Swapchain.extent.width);
    viewport.height = static_cast<float>(ctx.Swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = ctx.Swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &DescriptorSetLayout;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(ctx.Device, &pipeline_layout_info, nullptr,
                               &PipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a pipeline layout!");

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = PipelineLayout;
    pipeline_info.renderPass = RenderPass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(ctx.Device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                  &GraphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Graphics Pipeline!");

    vkDestroyShaderModule(ctx.Device, frag_module, nullptr);
    vkDestroyShaderModule(ctx.Device, vert_module, nullptr);
}

void TexturedQuadRenderer::CreateFramebuffers()
{
    Framebuffers.resize(SwapchainImageViews.size());

    for (size_t i = 0; i < SwapchainImageViews.size(); i++)
    {
        VkImageView attachments[] = {SwapchainImageViews[i]};

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = RenderPass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = ctx.Swapchain.extent.width;
        framebuffer_info.height = ctx.Swapchain.extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(ctx.Device, &framebuffer_info, nullptr,
                                &Framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create a framebuffer!");
    }
}

void TexturedQuadRenderer::CreateCommandPools()
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(ctx.Device, &pool_info, nullptr, &CommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");
}

void TexturedQuadRenderer::CreateCommandBuffers()
{
    CommandBuffers.resize(Framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)CommandBuffers.size();

    if (vkAllocateCommandBuffers(ctx.Device, &allocInfo, CommandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void TexturedQuadRenderer::SubmitCommandBuffers()
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

void TexturedQuadRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                               uint32_t imageIndex)
{
    UpdateUniformBuffer();

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = RenderPass;
    render_pass_info.framebuffer = Framebuffers[imageIndex];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = ctx.Swapchain.extent;

    VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          GraphicsPipeline);

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
                                PipelineLayout, 0, 1,
                                &DescriptorSets[FrameSemaphoreIndex], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(IndexCount), 1, 0, 0, 0);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);

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

    VertexCount = vertices.size();
    VkDeviceSize bufferSize = VertexCount * sizeof(Vertex);

    Buffer stagingBuffer;

    {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        stagingBuffer = Buffer::CreateBuffer(ctx, bufferSize, usage, properties);
    }

    void *data;
    vkMapMemory(ctx.Device, stagingBuffer.Memory, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(ctx.Device, stagingBuffer.Memory);

    {
        VkBufferUsageFlags usage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VertexBuffer = Buffer::CreateBuffer(ctx, bufferSize, usage, properties);
    }

    utils::CopyBufferInfo cp_info{
        .Queue = GraphicsQueue,
        .Pool = CommandPool,
        .Src = stagingBuffer.Handle,
        .Dst = VertexBuffer.Handle,
        .Size = bufferSize,
    };

    utils::CopyBuffer(ctx, cp_info);

    Buffer::DestroyBuffer(ctx, stagingBuffer);
}

void TexturedQuadRenderer::CreateIndexBuffers()
{
    // clang-format off
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };
    // clang-format on

    IndexCount = indices.size();
    VkDeviceSize bufferSize = sizeof(uint16_t) * IndexCount;

    Buffer stagingBuffer;

    {
        auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        auto properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        stagingBuffer = Buffer::CreateBuffer(ctx, bufferSize, usage, properties);
    }

    void *data;
    vkMapMemory(ctx.Device, stagingBuffer.Memory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(ctx.Device, stagingBuffer.Memory);

    {
        auto usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        auto properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        IndexBuffer = Buffer::CreateBuffer(ctx, bufferSize, usage, properties);
    }

    utils::CopyBufferInfo cp_info{
        .Queue = GraphicsQueue,
        .Pool = CommandPool,
        .Src = stagingBuffer.Handle,
        .Dst = IndexBuffer.Handle,
        .Size = bufferSize,
    };

    utils::CopyBuffer(ctx, cp_info);

    Buffer::DestroyBuffer(ctx, stagingBuffer);
}

void TexturedQuadRenderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        auto properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        UniformBuffers[i] = Buffer::CreateBuffer(ctx, bufferSize, usage, properties);

        vkMapMemory(ctx.Device, UniformBuffers[i].Memory, 0, bufferSize, 0,
                    &UniformBuffersMapped[i]);
    }
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

    UBOData.MVP = proj;

    std::memcpy(UniformBuffersMapped[FrameSemaphoreIndex], &UBOData, sizeof(UBOData));
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

    if (vkCreateDescriptorPool(ctx.Device, &poolInfo, nullptr, &DescriptorPool) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void TexturedQuadRenderer::CreateDescriptorSets()
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
        bufferInfo.buffer = UniformBuffers[i].Handle;
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

void TexturedQuadRenderer::CreateTextureImage()
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("assets/textures/texture.jpg", &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
        throw std::runtime_error("Failed to load texture image!");

    Buffer stagingBuffer;

    {
        auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        auto properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        stagingBuffer = Buffer::CreateBuffer(ctx, imageSize, usage, properties);
    }

    void *data;
    vkMapMemory(ctx.Device, stagingBuffer.Memory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(ctx.Device, stagingBuffer.Memory);

    stbi_image_free(pixels);

    {
        ImageInfo info{
            .Width = static_cast<uint32_t>(texWidth),
            .Height = static_cast<uint32_t>(texHeight),
            .Format = VK_FORMAT_R8G8B8A8_SRGB,
            .Tiling = VK_IMAGE_TILING_OPTIMAL,
            .Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        TextureImage = Image::CreateImage(ctx, info);
    }

    {
        utils::TransitionImageLayoutInfo info{
            .Queue = GraphicsQueue,
            .Pool = CommandPool,
            .Image = TextureImage.Handle,
            .Format = VK_FORMAT_R8G8B8A8_SRGB,
            .OldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .NewLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        };

        utils::TransitionImageLayout(ctx, info);
    }

    {
        utils::CopyBufferToImageInfo info{
            .Queue = GraphicsQueue,
            .Pool = CommandPool,
            .Buffer = stagingBuffer.Handle,
            .Image = TextureImage.Handle,
            .Width = static_cast<uint32_t>(texWidth),
            .Height = static_cast<uint32_t>(texHeight),
        };

        utils::CopyBufferToImage(ctx, info);
    }

    {
        utils::TransitionImageLayoutInfo info{
            .Queue = GraphicsQueue,
            .Pool = CommandPool,
            .Image = TextureImage.Handle,
            .Format = VK_FORMAT_R8G8B8A8_SRGB,
            .OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .NewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        utils::TransitionImageLayout(ctx, info);
    }

    Buffer::DestroyBuffer(ctx, stagingBuffer);
}

void TexturedQuadRenderer::CreateTextureImageView()
{
    TextureImageView = utils::CreateImageView(ctx, TextureImage.Handle, VK_FORMAT_R8G8B8A8_SRGB);
}

void TexturedQuadRenderer::CreateTextureSampler()
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