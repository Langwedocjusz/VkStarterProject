#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Image.h"
#include "Pipeline.h"

#include <glm/glm.hpp>

class TexturedCubeRenderer : public RendererBase {
  public:
    TexturedCubeRenderer(VulkanContext &ctx, std::function<void()> callback);

    ~TexturedCubeRenderer();

    void OnImGui() override;

  private:
    void CreateSwapchainResources() override;
    void DestroySwapchainResources() override;

    void SubmitCommandBuffers() override;

  private:
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipelines();

    void CreateCommandPools();
    void CreateCommandBuffers();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void CreateVertexBuffers();
    void CreateIndexBuffers();

    void CreateUniformBuffers();
    void UpdateUniformBuffer();

    void CreateDescriptorPool();
    void CreateDescriptorSets();

    void CreateTextureResources();

    void CreateDepthResources();

  private:
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    Pipeline mGraphicsPipeline;

    VkCommandPool mCommandPool;
    std::vector<VkCommandBuffer> mCommandBuffers;

    struct Vertex {
        glm::vec3 Pos;
        glm::vec2 TexCoord;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    Buffer mVertexBuffer;
    size_t mVertexCount;

    Buffer mIndexBuffer;
    size_t mIndexCount;

    std::vector<MappedUniformBuffer> mUniformBuffers;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float Phi = 0.0f;
    };
    UniformBufferObject mUBOData;

    Image mTextureImage;
    VkImageView mTextureImageView;
    VkSampler mTextureSampler;

    Image mDepthImage;
    VkImageView mDepthImageView;
};