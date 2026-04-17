/**
 * @file ImageLoader.cpp
 * @brief 图像加载/保存实现 — stb_image 后端
 * @author hxxcxx
 * @date 2026-04-17
 */

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// 安全设置：限制最大图像尺寸防止恶意文件占用过多内存
#define STBI_MAX_DIMENSIONS (16384)

#include "ImageLoader.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <string>

namespace MulanGeo::IO {

ImageData loadImage(std::string_view path, int forceChannels) {
    ImageData result;

    std::string pathStr(path);  // stbi 需要 null-terminated
    int w = 0, h = 0, ch = 0;
    unsigned char* raw = stbi_load(pathStr.c_str(), &w, &h, &ch, forceChannels);
    if (!raw) return result;

    int outChannels = (forceChannels > 0) ? forceChannels : ch;
    size_t totalBytes = static_cast<size_t>(w) * h * outChannels;

    result.width    = static_cast<uint32_t>(w);
    result.height   = static_cast<uint32_t>(h);
    result.channels = static_cast<uint32_t>(outChannels);
    result.pixels.assign(raw, raw + totalBytes);

    stbi_image_free(raw);
    return result;
}

ImageData loadImageFromMemory(const uint8_t* data, size_t size, int forceChannels) {
    ImageData result;
    if (!data || size == 0) return result;

    int w = 0, h = 0, ch = 0;
    unsigned char* raw = stbi_load_from_memory(
        data, static_cast<int>(size), &w, &h, &ch, forceChannels);
    if (!raw) return result;

    int outChannels = (forceChannels > 0) ? forceChannels : ch;
    size_t totalBytes = static_cast<size_t>(w) * h * outChannels;

    result.width    = static_cast<uint32_t>(w);
    result.height   = static_cast<uint32_t>(h);
    result.channels = static_cast<uint32_t>(outChannels);
    result.pixels.assign(raw, raw + totalBytes);

    stbi_image_free(raw);
    return result;
}

bool saveImagePNG(std::string_view path, const ImageData& image) {
    if (!image.valid()) return false;

    std::string pathStr(path);
    int stride = static_cast<int>(image.rowBytes());
    int ok = stbi_write_png(
        pathStr.c_str(),
        static_cast<int>(image.width),
        static_cast<int>(image.height),
        static_cast<int>(image.channels),
        image.pixels.data(),
        stride);
    return ok != 0;
}

} // namespace MulanGeo::IO
