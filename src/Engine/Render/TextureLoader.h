/**
 * @file TextureLoader.h
 * @brief 纹理加载器 — 从文件加载图片数据
 * @author hxxcxx
 * @date 2026-04-23
 */

#pragma once

#include "../RHI/Texture.h"
#include "../RHI/Device.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace MulanGeo::Engine {

// ============================================================
// 加载选项
// ============================================================

struct TextureLoadOptions {
    bool generateMips    = true;   // 是否生成 mipmap
    bool srgbToLinear   = false;  // 是否做 sRGB → Linear 转换
    TextureFormat format = TextureFormat::RGBA8_UNorm;
};

// ============================================================
// 纹理加载结果
// ============================================================

struct LoadedTexture {
    int                  width      = 0;
    int                  height     = 0;
    int                  channels   = 0;
    std::vector<uint8_t> pixels;    // 原始像素数据
    TextureFormat        format     = TextureFormat::RGBA8_UNorm;
};

// ============================================================
// 纹理加载器 — 单例或工具类
// ============================================================

class TextureLoader {
public:
    TextureLoader() = default;
    ~TextureLoader() = default;

    // 禁用拷贝
    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;

    /// 从文件加载纹理数据（使用 stb_image）
    /// 返回空 vector 表示加载失败
    LoadedTexture loadFromFile(const std::string& path,
                               const TextureLoadOptions& options = {}) const;

    /// 从内存加载纹理数据
    LoadedTexture loadFromMemory(const uint8_t* data,
                                  size_t size,
                                  const TextureLoadOptions& options = {}) const;

    /// 检查文件是否为支持的图片格式
    static bool isSupportedFormat(const std::string& path);

    /// 从文件路径推测格式
    static TextureFormat guessFormat(const std::string& path, int channels);
};

} // namespace MulanGeo::Engine
