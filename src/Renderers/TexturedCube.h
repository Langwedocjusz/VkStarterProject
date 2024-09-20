#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Image.h"
#include "Pipeline.h"

#include <glm/glm.hpp>

class TexturedCubeRenderer : public RendererBase {
  public:
    TexturedCubeRenderer(VulkanContext &ctx, std::function<void()> callback)
        : RendererBase(ctx, callback)
    {
    }
    ~TexturedCubeRenderer()
    {
        VulkanCleanup();
    }

    void OnImGui() override;

  private:
    void CreateResources() override;
    void CreateSwapchainResources() override;
    void CreateDependentResources() override;

    void DestroyResources() override;
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

    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();

    void CreateDepthResources();

  private:
    VkDescriptorSetLayout DescriptorSetLayout;
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;

    Pipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    struct Vertex {
        glm::vec3 Pos;
        glm::vec2 TexCoord;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    Buffer VertexBuffer;
    size_t VertexCount;

    Buffer IndexBuffer;
    size_t IndexCount;

    std::vector<MappedUniformBuffer> UniformBuffers;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float Phi = 0.0f;
    };
    UniformBufferObject UBOData;

    Image TextureImage;
    VkImageView TextureImageView;
    VkSampler TextureSampler;

    Image DepthImage;
    VkImageView DepthImageView;
};