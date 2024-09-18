#include "Pipeline.h"

PipelineBuilder::PipelineBuilder()
{
    mVertexInput = {};
    mInputAssembly = {};
    mRaster = {};
    mMultisample = {};
    mColorBlendAttachment = {};
    mColorBlend = {};
    mDepthStencil = {};

    mVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    mInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    mRaster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    mMultisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    mColorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    mDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    //Things that are hardcoded for now:
    mInputAssembly.primitiveRestartEnable = VK_FALSE;

    mRaster.depthClampEnable = VK_FALSE;
    mRaster.rasterizerDiscardEnable = VK_FALSE;
    mRaster.lineWidth = 1.0f;
    mRaster.depthBiasEnable = VK_FALSE;

    mMultisample.sampleShadingEnable = VK_FALSE;
    mMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    mColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    mColorBlendAttachment.blendEnable = VK_FALSE;

    mColorBlend.logicOpEnable = VK_FALSE;
    mColorBlend.logicOp = VK_LOGIC_OP_COPY;
    mColorBlend.attachmentCount = 1;
    mColorBlend.pAttachments = &mColorBlendAttachment;
    mColorBlend.blendConstants[0] = 0.0f;
    mColorBlend.blendConstants[1] = 0.0f;
    mColorBlend.blendConstants[2] = 0.0f;
    mColorBlend.blendConstants[3] = 0.0f;
}

PipelineBuilder PipelineBuilder::SetVertexInput(
        VkVertexInputBindingDescription &bindingDescription,
        std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
{
    mVertexInput.vertexBindingDescriptionCount = 1;
    mVertexInput.pVertexBindingDescriptions = &bindingDescription;

    mVertexInput.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    mVertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

    return *this;
}

PipelineBuilder PipelineBuilder::SetTopology(VkPrimitiveTopology topo)
{
    mInputAssembly.topology = topo;
    return *this;
}

PipelineBuilder PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
    mRaster.polygonMode = mode;
    return *this;
}

PipelineBuilder PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    mRaster.cullMode = cullMode;
    mRaster.frontFace = frontFace;
    return *this;
}

PipelineBuilder PipelineBuilder::DisableDepthTest()
{
    mDepthStencil.depthTestEnable = VK_FALSE;
    mDepthStencil.stencilTestEnable = VK_FALSE;
    return *this;
}

PipelineBuilder PipelineBuilder::EnableDepthTest()
{
    mDepthStencil.depthTestEnable = VK_TRUE;
    mDepthStencil.depthWriteEnable = VK_TRUE;
    mDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    mDepthStencil.depthBoundsTestEnable = VK_FALSE;
    mDepthStencil.minDepthBounds = 0.0f; // Optional
    mDepthStencil.maxDepthBounds = 1.0f; // Optional
    mDepthStencil.stencilTestEnable = VK_FALSE;
    mDepthStencil.front = {}; // Optional
    mDepthStencil.back = {};  // Optional
    return *this;
}

Pipeline PipelineBuilder::Build(VulkanContext& ctx, VkRenderPass renderPass, VkDescriptorSetLayout& descriptor)
{
    Pipeline pipeline;

    //Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptor;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(ctx.Device, &pipelineLayoutInfo, nullptr,
                               &pipeline.Layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a pipeline layout!");

    //Dynamic state
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

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicInfo.pDynamicStates = dynamicStates.data();

    //Pipeline creation
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(mShaderStages.size());
    pipelineInfo.pStages = mShaderStages.data();
    pipelineInfo.pVertexInputState = &mVertexInput;
    pipelineInfo.pInputAssemblyState = &mInputAssembly;
    pipelineInfo.pViewportState = &viewport_state;
    pipelineInfo.pRasterizationState = &mRaster;
    pipelineInfo.pMultisampleState = &mMultisample;
    pipelineInfo.pColorBlendState = &mColorBlend;
    pipelineInfo.pDepthStencilState = &mDepthStencil;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.layout = pipeline.Layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(ctx.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                  &pipeline.Handle) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Graphics Pipeline!");

    for (auto& shaderInfo : mShaderStages)
        vkDestroyShaderModule(ctx.Device, shaderInfo.module, nullptr);

    return pipeline;
}