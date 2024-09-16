#pragma once

#include "RendererBase.h"

#include <glm/glm.hpp>

class TexturedCubeRenderer : public RendererBase {
  public:
    TexturedCubeRenderer(VulkanContext& ctx, std::function<void()> callback) : RendererBase(ctx, callback) {}
    ~TexturedCubeRenderer() {VulkanCleanup();}

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

    void RecordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

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
    VkRenderPass RenderPass;

    VkDescriptorSetLayout DescriptorSetLayout;
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;

    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    struct Vertex {
        glm::vec3 Pos;
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

    VkImage DepthImage;
    VkDeviceMemory DepthImageMemory;
    VkImageView DepthImageView;
};