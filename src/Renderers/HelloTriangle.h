#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Pipeline.h"

#include <glm/glm.hpp>

class HelloTriangleRenderer : public RendererBase {
  public:
    HelloTriangleRenderer(VulkanContext &ctx, std::function<void()> callback);

    ~HelloTriangleRenderer();

    void OnImGui() override;

  private:
    void CreateSwapchainResources() override;
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
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    Pipeline mGraphicsPipeline;

    VkCommandPool mCommandPool;
    std::vector<VkCommandBuffer> mCommandBuffers;

    struct Vertex {
        glm::vec2 Pos;
        glm::vec3 Color;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    Buffer mVertexBuffer;
    size_t mVertexCount;

    std::vector<MappedUniformBuffer> mUniformBuffers;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float Phi = 0.0f;
    };
    UniformBufferObject mUBOData;
};