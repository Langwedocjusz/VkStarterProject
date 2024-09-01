#include "VulkanContext.h"

#include <iostream>

#include "Renderer.h"

int main()
{
    try
    {
        using namespace renderer;

        const uint32_t width = 800;
        const uint32_t height = 600;
        const std::string title = "Vulkan";

        VulkanContext ctx(width, height, title);
        RenderData data;

        CreateQueues(ctx, data);
        CreateRenderPass(ctx, data);
        CreateGraphicsPipeline(ctx, data);

        CreateFramebuffers(ctx, data);
        CreateCommandPool(ctx, data);
        CreateCommandBuffers(ctx, data);

        CreateSyncObjects(ctx, data);

        // Main Loop:
        while (!ctx.Window.ShouldClose())
        {
            ctx.Window.PollEvents();

            DrawFrame(ctx, data);
        }

        ctx.Disp.deviceWaitIdle();
        VulkanCleanup(ctx, data);
    }

    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }
}
