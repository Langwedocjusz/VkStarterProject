#include "HelloTriangle.h"

#include "Utils.h"

#include "ImGuiContext.h"
#include "imgui.h"

#include <array>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec2 Pos;
    glm::vec3 Color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, Pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, Color);

        return attributeDescriptions;
    }
};

void HelloTriangleRenderer::OnImGui()
{
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
}

void HelloTriangleRenderer::CreatePermanentResources(VulkanContext &ctx)
{
    CreateRenderPasses(ctx);
    CreateGraphicsPipelines(ctx);

    CreateVertexBuffers(ctx);
}

void HelloTriangleRenderer::CreateSwapchainResources(VulkanContext &ctx)
{
    CreateFramebuffers(ctx);
    CreateCommandPools(ctx);
    CreateCommandBuffers(ctx);
}

void HelloTriangleRenderer::DestroyPermanentResources(VulkanContext &ctx)
{
    ctx.Disp.destroyBuffer(VertexBuffer, nullptr);
    ctx.Disp.freeMemory(VertexBufferMemory, nullptr);

    ctx.Disp.destroyPipeline(GraphicsPipeline, nullptr);
    ctx.Disp.destroyPipelineLayout(PipelineLayout, nullptr);
    ctx.Disp.destroyRenderPass(RenderPass, nullptr);
}

void HelloTriangleRenderer::DestroySwapchainResources(VulkanContext &ctx)
{
    ctx.Disp.destroyCommandPool(CommandPool, nullptr);

    for (auto framebuffer : Framebuffers)
    {
        ctx.Disp.destroyFramebuffer(framebuffer, nullptr);
    }
}

void HelloTriangleRenderer::CreateRenderPasses(VulkanContext &ctx)
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

    if (ctx.Disp.createRenderPass(&render_pass_info, nullptr, &RenderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a render pass!");
}

void HelloTriangleRenderer::CreateGraphicsPipelines(VulkanContext &ctx)
{
    // Graphics pipeline:
    auto vert_code = utils::ReadFileBinary("assets/spirv/HelloTriangleVert.spv");
    auto frag_code = utils::ReadFileBinary("assets/spirv/HelloTriangleFrag.spv");

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
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (ctx.Disp.createPipelineLayout(&pipeline_layout_info, nullptr, &PipelineLayout) !=
        VK_SUCCESS)
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

    if (ctx.Disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                         &GraphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Graphics Pipeline!");

    ctx.Disp.destroyShaderModule(frag_module, nullptr);
    ctx.Disp.destroyShaderModule(vert_module, nullptr);
}

void HelloTriangleRenderer::CreateFramebuffers(VulkanContext &ctx)
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

        if (ctx.Disp.createFramebuffer(&framebuffer_info, nullptr, &Framebuffers[i]) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to create a framebuffer!");
    }
}

void HelloTriangleRenderer::CreateCommandPools(VulkanContext &ctx)
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex =
        ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (ctx.Disp.createCommandPool(&pool_info, nullptr, &CommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");
}

void HelloTriangleRenderer::CreateCommandBuffers(VulkanContext &ctx)
{
    CommandBuffers.resize(Framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)CommandBuffers.size();

    if (ctx.Disp.allocateCommandBuffers(&allocInfo, CommandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void HelloTriangleRenderer::SubmitCommandBuffers(VulkanContext &ctx)
{
    auto &imageAcquiredSemaphore = ImageAcquiredSemaphores[FrameSemaphoreIndex];
    auto &renderCompleteSemaphore = RenderCompletedSemaphores[FrameSemaphoreIndex];

    vkResetCommandBuffer(CommandBuffers[FrameSemaphoreIndex], 0);
    RecordCommandBuffer(ctx, CommandBuffers[FrameSemaphoreIndex], FrameImageIndex);

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

    if (ctx.Disp.queueSubmit(GraphicsQueue, 1, &submitInfo,
                             InFlightFences[FrameSemaphoreIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
}

void HelloTriangleRenderer::RecordCommandBuffer(VulkanContext &ctx,
                                                VkCommandBuffer commandBuffer,
                                                uint32_t imageIndex)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (ctx.Disp.beginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS)
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

    ctx.Disp.cmdBeginRenderPass(commandBuffer, &render_pass_info,
                                VK_SUBPASS_CONTENTS_INLINE);
    {
        ctx.Disp.cmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 GraphicsPipeline);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(ctx.Swapchain.extent.width);
        viewport.height = static_cast<float>(ctx.Swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        ctx.Disp.cmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = ctx.Swapchain.extent;
        ctx.Disp.cmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {VertexBuffer};
        VkDeviceSize offsets[] = {0};
        ctx.Disp.cmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        ctx.Disp.cmdDraw(commandBuffer, static_cast<uint32_t>(VertexCount), 1, 0, 0);

        ImGuiContextManager::RecordImguiToCommandBuffer(commandBuffer);
    }

    ctx.Disp.cmdEndRenderPass(commandBuffer);

    if (ctx.Disp.endCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
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

void HelloTriangleRenderer::CreateVertexBuffers(VulkanContext &ctx)
{
    // clang-format off
    const std::vector<Vertex> vertices{
        {{ 0.0f,-0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };
    // clang-format on

    VertexCount = vertices.size();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = VertexCount * sizeof(Vertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.Device, &bufferInfo, nullptr, &VertexBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.Device, VertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(ctx, memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(ctx.Device, &allocInfo, nullptr, &VertexBufferMemory) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(ctx.Device, VertexBuffer, VertexBufferMemory, 0);

    void* data;
    vkMapMemory(ctx.Device, VertexBufferMemory, 0, bufferInfo.size, 0, &data);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferInfo.size));
    vkUnmapMemory(ctx.Device, VertexBufferMemory);
}