/**
 * @file ImageData.h
 * @brief 图像数据容器 — CPU 端解码后的像素缓冲
 * @author hxxcxx
 * @date 2026-04-17
 *
 * 设计思路：
 *  - 值类型，可 move、可存入容器
 *  - 支持 1/2/3/4 通道（灰度 / 灰度+A / RGB / RGBA）
 *  - 像素按行排列，左上角原点（与 Vulkan/DX12 纹理上传一致）
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace MulanGeo::IO {

struct ImageData {
    uint32_t             width    = 0;
    uint32_t             height   = 0;
    uint32_t             channels = 0;   ///< 1=灰度, 2=灰度+A, 3=RGB, 4=RGBA
    std::vector<uint8_t> pixels;         ///< row-major, top-left origin

    bool valid() const {
        return width > 0 && height > 0 && channels > 0
            && pixels.size() == static_cast<size_t>(width) * height * channels;
    }

    /// 单行字节数
    uint32_t rowBytes() const { return width * channels; }

    /// 总字节数
    size_t totalBytes() const { return pixels.size(); }

    /// 翻转 Y 轴（某些格式如 BMP 底行在前，需要翻转）
    void flipVertically() {
        if (!valid()) return;
        uint32_t rowLen = rowBytes();
        std::vector<uint8_t> tmp(rowLen);
        for (uint32_t top = 0, bot = height - 1; top < bot; ++top, --bot) {
            uint8_t* pTop = pixels.data() + top * rowLen;
            uint8_t* pBot = pixels.data() + bot * rowLen;
            std::memcpy(tmp.data(), pTop, rowLen);
            std::memcpy(pTop, pBot, rowLen);
            std::memcpy(pBot, tmp.data(), rowLen);
        }
    }

    /// 强制转换为 RGBA（如果原始 < 4 通道则补齐）
    ImageData toRGBA() const {
        if (channels == 4) return *this;
        if (!valid()) return {};

        ImageData out;
        out.width    = width;
        out.height   = height;
        out.channels = 4;
        out.pixels.resize(static_cast<size_t>(width) * height * 4);

        size_t pixelCount = static_cast<size_t>(width) * height;
        for (size_t i = 0; i < pixelCount; ++i) {
            const uint8_t* src = pixels.data() + i * channels;
            uint8_t*       dst = out.pixels.data() + i * 4;

            switch (channels) {
            case 1: // 灰度
                dst[0] = dst[1] = dst[2] = src[0];
                dst[3] = 255;
                break;
            case 2: // 灰度 + Alpha
                dst[0] = dst[1] = dst[2] = src[0];
                dst[3] = src[1];
                break;
            case 3: // RGB
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
                dst[3] = 255;
                break;
            default:
                break;
            }
        }
        return out;
    }
};

} // namespace MulanGeo::IO
