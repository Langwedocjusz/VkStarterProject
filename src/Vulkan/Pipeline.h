#pragma once

#include "VulkanContext.h"

#include <vector>

struct Pipeline {
    VkPipeline Handle;
    VkPipelineLayout Layout;
};

class PipelineBuilder {
  public:
    PipelineBuilder();

    // Temporary hack
    PipelineBuilder SetShaderStages(std::vector<VkPipelineShaderStageCreateInfo> stages)
    {
        mShaderStages = stages;
        return *this;
    }

    PipelineBuilder SetVertexInput(
        VkVertexInputBindingDescription &bindingDescription,
        std::vector<VkVertexInputAttributeDescription> &attributeDescriptions);
    PipelineBuilder SetTopology(VkPrimitiveTopology topo);
    PipelineBuilder SetPolygonMode(VkPolygonMode mode);
    PipelineBuilder SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    PipelineBuilder DisableDepthTest();
    PipelineBuilder EnableDepthTest();

    PipelineBuilder SetSwapchainColorFormat(VkFormat format);
    PipelineBuilder SetDepthFormat(VkFormat format);

    PipelineBuilder EnableBlending();

    Pipeline Build(VulkanContext &ctx, VkDescriptorSetLayout &descriptor);

  private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;

    VkPipelineVertexInputStateCreateInfo mVertexInput;
    VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
    VkPipelineRasterizationStateCreateInfo mRaster;
    VkPipelineMultisampleStateCreateInfo mMultisample;
    VkPipelineColorBlendAttachmentState mColorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo mColorBlend;
    VkPipelineDepthStencilStateCreateInfo mDepthStencil;

    VkFormat mSwapchainColorFormat;

    bool mDepthFormatProvided = false;
    VkFormat mDepthFormat;
};

class ComputePipelineBuilder {
  public:
    ComputePipelineBuilder() = default;

    ComputePipelineBuilder SetShaderStage(VkPipelineShaderStageCreateInfo stage)
    {
        mShaderStage = stage;
        return *this;
    }

    Pipeline Build(VulkanContext &ctx, VkDescriptorSetLayout &descriptor);

  private:
    VkPipelineShaderStageCreateInfo mShaderStage;
};