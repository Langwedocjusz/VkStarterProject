#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Image.h"
#include "Pipeline.h"

#include <glm/glm.hpp>

class ModelRenderer : public RendererBase {
  public:
    ModelRenderer(VulkanContext &ctx, std::function<void()> callback);

    ~ModelRenderer();

    void OnUpdate() override;
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

    void LoadModel();
    void CreateUniformBuffers();

    void CreateTextureResources();
    void CreateDepthResources();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

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

    struct GeoSurface {
        uint32_t StartIndex;
        uint32_t Count;
    };

    std::vector<GeoSurface> mSurfaces;

    std::vector<MappedUniformBuffer> mUniformBuffers;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
    };
    UniformBufferObject mUBOData;

    float mRotationAngle = 0.0f;
    float mCameraDistance = 3.0f;

    Image mTextureImage;
    VkImageView mTextureImageView;
    VkSampler mTextureSampler;

    Image mDepthImage;
    VkImageView mDepthImageView;
};