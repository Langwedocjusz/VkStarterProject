#pragma once

#include "RendererBase.h"

#include "Buffer.h"

#include <glm/glm.hpp>

class HelloTriangleRenderer : public RendererBase {
  public:
    HelloTriangleRenderer(VulkanContext &ctx, std::function<void()> callback)
        : RendererBase(ctx, callback)
    {
    }
    ~HelloTriangleRenderer()
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

    void CreateUniformBuffers();
    void UpdateUniformBuffer();

    void CreateDescriptorPool();
    void CreateDescriptorSets();

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
        glm::vec3 Color;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2>
        getAttributeDescriptions();
    };

    Buffer VertexBuffer;
    size_t VertexCount;

    std::vector<MappedUniformBuffer> UniformBuffers;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float Phi = 0.0f;
    };
    UniformBufferObject UBOData;
};