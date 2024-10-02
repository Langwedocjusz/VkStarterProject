#include "ImageLoaders.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image ImageLoaders::LoadImage2D(VulkanContext &ctx, ImageLoaderInfo &info)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(info.Filepath.c_str(), &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);

    if (!pixels)
    {
        std::string err_msg = "Failed to load texture image!\n";
        err_msg += "Filepath: " + info.Filepath;

        throw std::runtime_error(err_msg);
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    ImageInfo img_info{
        .Width = static_cast<uint32_t>(texWidth),
        .Height = static_cast<uint32_t>(texHeight),
        .Format = VK_FORMAT_R8G8B8A8_SRGB,
        .Tiling = VK_IMAGE_TILING_OPTIMAL,
        .Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    Image img = Image::CreateImage(ctx, img_info);

    ImageDataInfo data_info{
        .Queue = info.Queue,
        .Pool = info.Pool,
        .Data = pixels,
        .Size = imageSize,
    };

    Image::UploadToImage(ctx, img, data_info);

    stbi_image_free(pixels);

    return img;
}