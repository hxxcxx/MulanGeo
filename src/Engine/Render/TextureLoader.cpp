/**
 * @file TextureLoader.cpp
 * @brief 纹理加载器实现
 */

#include "TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <filesystem>

namespace MulanGeo::Engine {

LoadedTexture TextureLoader::loadFromFile(const std::string& path,
                                          const TextureLoadOptions& options) const {
    LoadedTexture result;

    // 加载图片
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);

    uint8_t* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_default);
    if (!pixels) {
        return {};
    }

    result.width    = width;
    result.height   = height;
    result.channels = channels;
    result.format   = options.format;

    size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    result.pixels.resize(pixelCount);

    // 转换到目标格式
    if (channels == 1) {
        // 灰度 → RGBA
        for (size_t i = 0; i < static_cast<size_t>(width * height); ++i) {
            result.pixels[i * 4 + 0] = pixels[i];
            result.pixels[i * 4 + 1] = pixels[i];
            result.pixels[i * 4 + 2] = pixels[i];
            result.pixels[i * 4 + 3] = 255;
        }
    } else if (channels == 3) {
        // RGB → RGBA
        for (size_t i = 0; i < static_cast<size_t>(width * height); ++i) {
            result.pixels[i * 4 + 0] = pixels[i * 3 + 0];
            result.pixels[i * 4 + 1] = pixels[i * 3 + 1];
            result.pixels[i * 4 + 2] = pixels[i * 3 + 2];
            result.pixels[i * 4 + 3] = 255;
        }
    } else if (channels == 4) {
        std::memcpy(result.pixels.data(), pixels, pixelCount);
    } else {
        stbi_image_free(pixels);
        return {};
    }

    // sRGB → Linear 转换（如果需要）
    if (options.srgbToLinear) {
        for (size_t i = 0; i < result.pixels.size(); i += 4) {
            auto srgb_to_linear = [](uint8_t srgb) -> float {
                float v = srgb / 255.0f;
                return (v <= 0.04045f) ? v / 12.92f
                                        : std::pow((v + 0.055f) / 1.055f, 2.4f);
            };
            result.pixels[i + 0] = static_cast<uint8_t>(srgb_to_linear(result.pixels[i + 0]) * 255);
            result.pixels[i + 1] = static_cast<uint8_t>(srgb_to_linear(result.pixels[i + 1]) * 255);
            result.pixels[i + 2] = static_cast<uint8_t>(srgb_to_linear(result.pixels[i + 2]) * 255);
        }
    }

    stbi_image_free(pixels);
    return result;
}

LoadedTexture TextureLoader::loadFromMemory(const uint8_t* data,
                                             size_t size,
                                             const TextureLoadOptions& options) const {
    LoadedTexture result;

    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);

    uint8_t* pixels = stbi_load_from_memory(data, static_cast<int>(size),
                                              &width, &height, &channels, STBI_default);
    if (!pixels) {
        return {};
    }

    result.width    = width;
    result.height   = height;
    result.channels = channels;
    result.format   = options.format;

    size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    result.pixels.resize(pixelCount);

    if (channels == 1) {
        for (size_t i = 0; i < static_cast<size_t>(width * height); ++i) {
            result.pixels[i * 4 + 0] = pixels[i];
            result.pixels[i * 4 + 1] = pixels[i];
            result.pixels[i * 4 + 2] = pixels[i];
            result.pixels[i * 4 + 3] = 255;
        }
    } else if (channels == 3) {
        for (size_t i = 0; i < static_cast<size_t>(width * height); ++i) {
            result.pixels[i * 4 + 0] = pixels[i * 3 + 0];
            result.pixels[i * 4 + 1] = pixels[i * 3 + 1];
            result.pixels[i * 4 + 2] = pixels[i * 3 + 2];
            result.pixels[i * 4 + 3] = 255;
        }
    } else if (channels == 4) {
        std::memcpy(result.pixels.data(), pixels, pixelCount);
    }

    stbi_image_free(pixels);
    return result;
}

bool TextureLoader::isSupportedFormat(const std::string& path) {
    namespace fs = std::filesystem;
    fs::path p(path);
    auto ext = p.extension().string();
    // 转为小写
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg"
        || ext == ".bmp" || ext == ".tga" || ext == ".hdr";
}

TextureFormat TextureLoader::guessFormat(const std::string& path, int channels) {
    namespace fs = std::filesystem;
    fs::path p(path);
    auto ext = p.extension().string();
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    bool isSrgb = (ext == ".jpg" || ext == ".jpeg");

    if (channels == 1) return TextureFormat::R8_UNorm;
    if (channels == 3) return isSrgb ? TextureFormat::RGBA8_sRGB : TextureFormat::RGBA8_UNorm;
    if (channels == 4) return isSrgb ? TextureFormat::RGBA8_sRGB : TextureFormat::RGBA8_UNorm;

    return TextureFormat::RGBA8_UNorm;
}

} // namespace MulanGeo::Engine
