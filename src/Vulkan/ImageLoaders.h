#pragma once

#include "Image.h"

#include <string>

struct ImageLoaderInfo{
    VkQueue Queue;
    VkCommandPool Pool;
    std::string Filepath;
};

namespace ImageLoaders{
    Image LoadImage2D(VulkanContext& ctx, ImageLoaderInfo& info);
}