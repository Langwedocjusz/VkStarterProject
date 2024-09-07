#include "Renderer.h"

#include "Utils.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>

static VkShaderModule CreateShaderModule(VulkanContext &ctx, const std::vector<char> &code)
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

namespace renderer
{
void CreateQueues(VulkanContext &ctx, RenderData &data)
{
    auto gq = ctx.Device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value())
    {
        auto err_msg = "Failed to get graphics queue: " + gq.error().message();
        throw std::runtime_error(err_msg);
    }
    data.GraphicsQueue = gq.value();

    auto pq = ctx.Device.get_queue(vkb::QueueType::present);
    if (!pq.has_value())
    {
        auto err_msg = "Failed to get present queue: " + pq.error().message();
        throw std::runtime_error(err_msg);
    }
    data.PresentQueue = pq.value();
}

void CreateRenderPass(VulkanContext &ctx, RenderData &data)
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
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (ctx.Disp.createRenderPass(&render_pass_info, nullptr, &data.RenderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a render pass!");
}

void CreateGraphicsPipeline(VulkanContext &ctx, RenderData &data)
{
    // Graphics pipeline:
    auto vert_code = utils::ReadFileBinary("assets/spirv/HelloTriangleVert.spv");
    auto frag_code = utils::ReadFileBinary("assets/spirv/HelloTriangleFrag.spv");

    VkShaderModule vert_module = CreateShaderModule(ctx, vert_code);
    VkShaderModule frag_module = CreateShaderModule(ctx, frag_code);

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

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

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
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

    if (ctx.Disp.createPipelineLayout(&pipeline_layout_info, nullptr, &data.PipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a pipeline layout!");

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

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
    pipeline_info.layout = data.PipelineLayout;
    pipeline_info.renderPass = data.RenderPass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (ctx.Disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &data.GraphicsPipeline) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create a Graphics Pipeline!");

    ctx.Disp.destroyShaderModule(frag_module, nullptr);
    ctx.Disp.destroyShaderModule(vert_module, nullptr);
}

void CreateSyncObjects(VulkanContext &ctx, RenderData &data)
{
    data.ImageAcquiredSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    data.RenderCompletedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    data.InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    data.ImageInFlight.resize(ctx.Swapchain.image_count, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (ctx.Disp.createSemaphore(&semaphore_info, nullptr, &data.ImageAcquiredSemaphores[i]) != VK_SUCCESS ||
            ctx.Disp.createSemaphore(&semaphore_info, nullptr, &data.RenderCompletedSemaphores[i]) != VK_SUCCESS ||
            ctx.Disp.createFence(&fence_info, nullptr, &data.InFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects");
    }
}

void CreateFramebuffers(VulkanContext &ctx, RenderData &data)
{
    data.SwapchainImages = ctx.Swapchain.get_images().value();
    data.SwapchainImageViews = ctx.Swapchain.get_image_views().value();

    data.Framebuffers.resize(data.SwapchainImageViews.size());

    for (size_t i = 0; i < data.SwapchainImageViews.size(); i++)
    {
        VkImageView attachments[] = {data.SwapchainImageViews[i]};

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = data.RenderPass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = ctx.Swapchain.extent.width;
        framebuffer_info.height = ctx.Swapchain.extent.height;
        framebuffer_info.layers = 1;

        if (ctx.Disp.createFramebuffer(&framebuffer_info, nullptr, &data.Framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create a framebuffer!");
    }
}

void CreateCommandPool(VulkanContext &ctx, RenderData &data)
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = ctx.Device.get_queue_index(vkb::QueueType::graphics).value();

    if (ctx.Disp.createCommandPool(&pool_info, nullptr, &data.CommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a command pool!");
}

void CreateCommandBuffers(VulkanContext &ctx, RenderData &data)
{
    data.CommandBuffers.resize(data.Framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = data.CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)data.CommandBuffers.size();

    if (ctx.Disp.allocateCommandBuffers(&allocInfo, data.CommandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");

    for (size_t i = 0; i < data.CommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (ctx.Disp.beginCommandBuffer(data.CommandBuffers[i], &begin_info) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = data.RenderPass;
        render_pass_info.framebuffer = data.Framebuffers[i];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = ctx.Swapchain.extent;
        VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

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

        ctx.Disp.cmdSetViewport(data.CommandBuffers[i], 0, 1, &viewport);
        ctx.Disp.cmdSetScissor(data.CommandBuffers[i], 0, 1, &scissor);

        ctx.Disp.cmdBeginRenderPass(data.CommandBuffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        ctx.Disp.cmdBindPipeline(data.CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.GraphicsPipeline);

        ctx.Disp.cmdDraw(data.CommandBuffers[i], 3, 1, 0, 0);

        ctx.Disp.cmdEndRenderPass(data.CommandBuffers[i]);

        if (ctx.Disp.endCommandBuffer(data.CommandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    }
}

void DrawFrame(VulkanContext &ctx, RenderData &data)
{
    ctx.Disp.waitForFences(1, &data.InFlightFences[data.FrameSemaphoreIndex], VK_TRUE, UINT64_MAX);

    auto& imageAcquiredSemaphore = data.ImageAcquiredSemaphores[data.FrameSemaphoreIndex];
    auto& renderCompleteSemaphore = data.RenderCompletedSemaphores[data.FrameSemaphoreIndex];

    if (ctx.SwapchainOk)
    {
        VkResult result =
            ctx.Disp.acquireNextImageKHR(ctx.Swapchain, UINT64_MAX, imageAcquiredSemaphore,
                                         VK_NULL_HANDLE, &data.FrameImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            ctx.SwapchainOk = false;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }
    }

    if (!ctx.SwapchainOk)
        return;

    if (data.ImageInFlight[data.FrameImageIndex] != VK_NULL_HANDLE)
    {
        ctx.Disp.waitForFences(1, &data.ImageInFlight[data.FrameImageIndex], VK_TRUE, UINT64_MAX);
    }
    data.ImageInFlight[data.FrameImageIndex] = data.InFlightFences[data.FrameSemaphoreIndex];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {imageAcquiredSemaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &data.CommandBuffers[data.FrameImageIndex];

    VkSemaphore signal_semaphores[] = {renderCompleteSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    ctx.Disp.resetFences(1, &data.InFlightFences[data.FrameSemaphoreIndex]);

    // Re-record command buffer
    {
        auto res = vkResetCommandPool(ctx.Device, data.CommandPool, 0);

        if (res != VK_SUCCESS)
            throw std::runtime_error("Failed to reset command pool!");

        // to-do: maybe clean this code duplication up, may not be worth it, since we'll switch to dynamic rendering in
        // the future
        for (size_t i = 0; i < data.CommandBuffers.size(); i++)
        {
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //new

            if (ctx.Disp.beginCommandBuffer(data.CommandBuffers[i], &begin_info) != VK_SUCCESS)
                throw std::runtime_error("Failed to begin recording command buffer!");

            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = data.RenderPass;
            render_pass_info.framebuffer = data.Framebuffers[i];
            render_pass_info.renderArea.offset = {0, 0};
            render_pass_info.renderArea.extent = ctx.Swapchain.extent;
            VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
            render_pass_info.clearValueCount = 1;
            render_pass_info.pClearValues = &clearColor;

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

            ctx.Disp.cmdSetViewport(data.CommandBuffers[i], 0, 1, &viewport);
            ctx.Disp.cmdSetScissor(data.CommandBuffers[i], 0, 1, &scissor);

            ctx.Disp.cmdBeginRenderPass(data.CommandBuffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            ctx.Disp.cmdBindPipeline(data.CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.GraphicsPipeline);

            ctx.Disp.cmdDraw(data.CommandBuffers[i], 3, 1, 0, 0);

            // Record dear imgui primitives into command buffer
            ImDrawData* draw_data = ImGui::GetDrawData();
            if (draw_data)
                ImGui_ImplVulkan_RenderDrawData(draw_data, data.CommandBuffers[i]);

            ctx.Disp.cmdEndRenderPass(data.CommandBuffers[i]);

            if (ctx.Disp.endCommandBuffer(data.CommandBuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to record command buffer!");
        }

        if (ctx.Disp.queueSubmit(data.GraphicsQueue, 1, &submitInfo, data.InFlightFences[data.FrameSemaphoreIndex]) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }
    }
}

    void PresentFrame(VulkanContext & ctx, RenderData & data)
    {
        if (!ctx.SwapchainOk)
            return;

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signal_semaphores[] = {data.RenderCompletedSemaphores[data.FrameSemaphoreIndex]};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR swapChains[] = {ctx.Swapchain};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapChains;

        present_info.pImageIndices = &data.FrameImageIndex;

        VkResult result = ctx.Disp.queuePresentKHR(data.PresentQueue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            ctx.SwapchainOk = false;
            return;
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain image!");
        }

        data.FrameSemaphoreIndex = (data.FrameSemaphoreIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void RecreateSwapchain(VulkanContext & ctx, RenderData & data)
    {
        ctx.Disp.deviceWaitIdle();

        ctx.Disp.destroyCommandPool(data.CommandPool, nullptr);

        for (auto framebuffer : data.Framebuffers)
        {
            ctx.Disp.destroyFramebuffer(framebuffer, nullptr);
        }

        ctx.Swapchain.destroy_image_views(data.SwapchainImageViews);

        ctx.CreateSwapchain(ctx.Width, ctx.Height);

        CreateFramebuffers(ctx, data);
        CreateCommandPool(ctx, data);
        CreateCommandBuffers(ctx, data);

        ctx.SwapchainOk = true;

        DrawFrame(ctx, data);
        PresentFrame(ctx, data);
    }

    void VulkanCleanup(VulkanContext & ctx, RenderData & data)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            ctx.Disp.destroySemaphore(data.RenderCompletedSemaphores[i], nullptr);
            ctx.Disp.destroySemaphore(data.ImageAcquiredSemaphores[i], nullptr);
            ctx.Disp.destroyFence(data.InFlightFences[i], nullptr);
        }

        ctx.Disp.destroyCommandPool(data.CommandPool, nullptr);

        for (auto framebuffer : data.Framebuffers)
        {
            ctx.Disp.destroyFramebuffer(framebuffer, nullptr);
        }

        ctx.Disp.destroyPipeline(data.GraphicsPipeline, nullptr);
        ctx.Disp.destroyPipelineLayout(data.PipelineLayout, nullptr);
        ctx.Disp.destroyRenderPass(data.RenderPass, nullptr);

        ctx.Swapchain.destroy_image_views(data.SwapchainImageViews);

        vkb::destroy_swapchain(ctx.Swapchain);
        vkb::destroy_device(ctx.Device);
        vkb::destroy_surface(ctx.Instance, ctx.Surface);
        vkb::destroy_instance(ctx.Instance);
    }
} // namespace renderer