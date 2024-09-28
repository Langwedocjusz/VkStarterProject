#pragma once

#include "RendererBase.h"

#include "Buffer.h"
#include "Pipeline.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

class ComputeParticleRenderer : public RendererBase {
  public:
    ComputeParticleRenderer(VulkanContext &ctx, std::function<void()> callback);

    ~ComputeParticleRenderer();

    void OnUpdate() override;
    void OnImGui() override;
    void OnRenderImpl() override;

  private:
    void CreateSwapchainResources() override;

  private:
    void CreateDescriptorSets();
    void UpdateDescriptorSets();
    void CreateGraphicsPipelines();
    void CreateComputePipelines();

    void CreateCommandPools();
    void CreateCommandBuffers();

    void CreateSyncObjects();

    void CreateVertexBuffers();
    void CreateUniformBuffers();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer);

  private:
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    Pipeline mGraphicsPipeline;
    Pipeline mComputePipeline;

    VkCommandPool mCommandPool;
    std::vector<VkCommandBuffer> mCommandBuffers;
    std::vector<VkCommandBuffer> mComputeCommandBuffers;

    struct Vertex {
        glm::vec2 Pos;
        glm::vec2 Velocity;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    std::vector<Buffer> mVertexBuffers;
    size_t mVertexCount;

    std::vector<MappedUniformBuffer> mUniformBuffers;

    struct UniformBufferObject {
        glm::mat4 MVP = glm::mat4(1.0f);
        float PointSize = 50.0f;
        float Speed = 0.5f;
    };
    UniformBufferObject mUBOData;

    std::vector<VkSemaphore> mComputeFinishedSemaphores;
    std::vector<VkFence> mComputeInFlightFences;
};