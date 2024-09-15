#pragma once

#include "RendererBase.h"

#include <glm/glm.hpp>

class TexturedQuadRenderer : public RendererBase {
  public:
    void OnImGui() override;

  private:
    VkRenderPass getImGuiRenderPass() const override
    {
        return RenderPass;
    }

    void CreateResources(VulkanContext &ctx) override;
    void CreateSwapchainResources(VulkanContext &ctx) override;
    void CreateDependentResources(VulkanContext &ctx) override;

    void DestroyResources(VulkanContext &ctx) override;
    void DestroySwapchainResources(VulkanContext &ctx) override;

    void SubmitCommandBuffers(VulkanContext &ctx) override;

  private:
    void CreateDescriptorSetLayout(VulkanContext &ctx);
    void CreateRenderPasses(VulkanContext &ctx);
    void CreateGraphicsPipelines(VulkanContext &ctx);

    void CreateCommandPools(VulkanContext &ctx);
    void CreateCommandBuffers(VulkanContext &ctx);
    void CreateFramebuffers(VulkanContext &ctx);

    void RecordCommandBuffer(VulkanContext &ctx, VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void CreateVertexBuffers(VulkanContext &ctx);
    void CreateIndexBuffers(VulkanContext &ctx);

    void CreateUniformBuffers(VulkanContext &ctx);
    void UpdateUniformBuffer(VulkanContext &ctx);

    void CreateDescriptorPool(VulkanContext &ctx);
    void CreateDescriptorSets(VulkanContext &ctx);

    void CreateTextureImage(VulkanContext &ctx);
    void CreateTextureImageView(VulkanContext &ctx);
    void CreateTextureSampler(VulkanContext &ctx);

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

    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    size_t VertexCount;

    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;
    size_t IndexCount;

    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void *> UniformBuffersMapped;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float Phi = 0.0f;
    };
    UniformBufferObject UBOData;

    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureSampler;
};