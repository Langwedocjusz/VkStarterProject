#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Image.h"

#include <glm/glm.hpp>

class TexturedQuadRenderer : public RendererBase {
  public:
    TexturedQuadRenderer(VulkanContext &ctx, std::function<void()> callback)
        : RendererBase(ctx, callback)
    {
    }
    ~TexturedQuadRenderer()
    {
        VulkanCleanup();
    }

    void OnImGui() override;

  private:
    VkRenderPass getImGuiRenderPass() const override
    {
        return RenderPass;
    }

    void CreateResources() override;
    void CreateSwapchainResources() override;
    void CreateDependentResources() override;

    void DestroyResources() override;
    void DestroySwapchainResources() override;

    void SubmitCommandBuffers() override;

  private:
    void CreateDescriptorSetLayout();
    void CreateRenderPasses();
    void CreateGraphicsPipelines();

    void CreateCommandPools();
    void CreateCommandBuffers();
    void CreateFramebuffers();

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

  private:
    VkRenderPass RenderPass;

    VkDescriptorSetLayout DescriptorSetLayout;
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;

    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    struct Vertex {
        glm::vec2 Pos;
        glm::vec2 TexCoord;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2>
        getAttributeDescriptions();
    };

    Buffer VertexBuffer;
    size_t VertexCount;

    Buffer IndexBuffer;
    size_t IndexCount;

    std::vector<Buffer> UniformBuffers;
    std::vector<void *> UniformBuffersMapped;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float Phi = 0.0f;
    };
    UniformBufferObject UBOData;

    Image TextureImage;
    VkImageView TextureImageView;
    VkSampler TextureSampler;
};