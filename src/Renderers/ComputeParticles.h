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

    void OnImGui() override;

  private:
    void CreateSwapchainResources() override;
    void DestroySwapchainResources() override;

    void SubmitCommandBuffers() override;

    void SubmitCommandBuffersEarly() override;

  private:
    void CreateDescriptorSets();
    void CreateGraphicsPipelines();
    void CreateComputePipelines();

    void CreateCommandPools();
    void CreateCommandBuffers();
    void CreateComputeCommandBuffers();

    void CreateMoreSyncObjects();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer);

    void CreateVertexBuffers();

    void CreateUniformBuffers();
    void UpdateUniformBuffer();

    void UpdateDescriptorSets();

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