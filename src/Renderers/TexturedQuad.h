#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Image.h"
#include "Pipeline.h"

#include <glm/glm.hpp>

class TexturedQuadRenderer : public RendererBase {
  public:
    TexturedQuadRenderer(VulkanContext &ctx, std::function<void()> callback);

    ~TexturedQuadRenderer();

    void OnUpdate([[maybe_unused]] float deltatime) override;
    void OnImGui() override;
    void OnRenderImpl() override;

  private:
    void CreateSwapchainResources() override;

  private:
    void CreateDescriptorSets();
    void UpdateDescriptorSets();
    void CreateGraphicsPipelines();

    void CreateCommandPools();
    void CreateCommandBuffers();

    void CreateVertexBuffers();
    void CreateIndexBuffers();
    void CreateUniformBuffers();

    void CreateTextureResources();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  private:
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    Pipeline mGraphicsPipeline;

    VkCommandPool mCommandPool;
    std::vector<VkCommandBuffer> mCommandBuffers;

    struct Vertex {
        glm::vec2 Pos;
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
};