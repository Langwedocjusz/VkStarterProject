#pragma once

#include "VulkanContext.h"

#include <vector>

struct Pipeline{
    VkPipeline Handle;
    VkPipelineLayout Layout;
};

class PipelineBuilder{
public:
    PipelineBuilder();

    //Temporary hack
    PipelineBuilder SetShaderStages(std::vector<VkPipelineShaderStageCreateInfo> stages)
    {
        mShaderStages = stages;
        return *this;
    }

    PipelineBuilder SetVertexInput(VkVertexInputBindingDescription& bindingDescription,
        std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
    PipelineBuilder SetTopology(VkPrimitiveTopology topo);
    PipelineBuilder SetPolygonMode(VkPolygonMode mode);
    PipelineBuilder SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    PipelineBuilder DisableDepthTest();
    PipelineBuilder EnableDepthTest();

    Pipeline Build(VulkanContext& ctx, VkRenderPass renderPass, VkDescriptorSetLayout& descriptor);

private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;

    VkPipelineVertexInputStateCreateInfo mVertexInput;
    VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
    VkPipelineRasterizationStateCreateInfo mRaster;
    VkPipelineMultisampleStateCreateInfo mMultisample;
    VkPipelineColorBlendAttachmentState mColorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo mColorBlend;
    VkPipelineDepthStencilStateCreateInfo mDepthStencil;
};