/**
 * @file ImageLoader.h
 * @brief 图像文件加载器接口
 * @author hxxcxx
 * @date 2026-04-17
 *
 * 设计思路：
 *  - 仅声明接口，实现在 ImageLoader.cpp 中（链接 stb_image）
 *  - 支持 PNG / JPG / BMP / TGA / HDR 常见格式
 *  - loadFromFile: 从磁盘路径加载
 *  - loadFromMemory: 从内存缓冲加载（嵌入式资源）
 *
 * 使用 stb_image 单头文件库作为后端，通过 vcpkg 引入。
 */

#pragma once

#include "ImageData.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#if defined(IO_EXPORTS) || defined(BUILDING_IO)
#   define IO_API __declspec(dllexport)
#else
#   define IO_API __declspec(dllimport)
#endif

namespace MulanGeo::IO {

/**
 * @brief 从文件路径加载图像
 * @param path 文件路径（UTF-8）
 * @param forceChannels 强制输出通道数（0=自动检测，4=强制RGBA）
 * @return ImageData 成功时 valid()==true，失败时 width=height=0
 */
IO_API ImageData loadImage(std::string_view path, int forceChannels = 0);

/**
 * @brief 从内存缓冲加载图像
 * @param data 图像文件原始字节
 * @param size 字节长度
 * @param forceChannels 强制输出通道数
 */
IO_API ImageData loadImageFromMemory(const uint8_t* data, size_t size, int forceChannels = 0);

/**
 * @brief 将图像保存为 PNG 文件
 * @param path 输出路径（UTF-8）
 * @param image 图像数据
 * @return true 写入成功
 */
IO_API bool saveImagePNG(std::string_view path, const ImageData& image);

} // namespace MulanGeo::IO
