/**
 * @file Material.h
 * @brief 材质类型系统与 GPU 数据布局
 * @author hxxcxx
 * @date 2026-04-17
 *
 * 设计思路：
 *  - Material 是值类型，可直接存入容器或节点
 *  - MaterialType 枚举标识着色器变体选择
 *  - MaterialGPU 是 std140 布局的 GPU 常量缓冲区数据
 *  - 上层通过 Material::toGPU() 转换后上传 UBO
 */

#pragma once

#include "../Math/Math.h"

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>

namespace MulanGeo::Engine {

// ============================================================
// 材质渲染类型 — 决定着色器选择
// ============================================================

enum class MaterialType : uint8_t {
    Unlit,          // 无光照（调试、线框叠加）
    BlinnPhong,     // 经典 Blinn-Phong 光照
    PBR,            // 金属度-粗糙度 PBR 工作流
};

// ============================================================
// Alpha 模式 — 透明度处理策略
// ============================================================

enum class AlphaMode : uint8_t {
    Opaque,         // 完全不透明
    Mask,           // 二值蒙版（alpha < cutoff 则丢弃）
    Blend,          // 半透明混合
};

// ============================================================
// 材质描述 — CPU 端完整参数
// ============================================================

struct Material {
    std::string     name;

    MaterialType    type        = MaterialType::PBR;
    AlphaMode       alphaMode   = AlphaMode::Opaque;

    // --- 基础颜色 (sRGB 空间) ---
    Vec3            baseColor   = {0.8, 0.8, 0.8};
    double          alpha       = 1.0;

    // --- PBR 参数 ---
    double          metallic    = 0.0;    ///< 金属度 [0, 1]
    double          roughness   = 0.5;    ///< 粗糙度 [0, 1]
    double          ao          = 1.0;    ///< 环境光遮蔽 [0, 1]

    // --- Blinn-Phong 参数 ---
    Vec3            specular    = {0.5, 0.5, 0.5};
    double          shininess   = 32.0;

    // --- Emissive ---
    Vec3            emissive    = Vec3(0.0);
    double          emissiveStrength = 1.0;

    // --- Alpha Mask ---
    double          alphaCutoff = 0.5;

    // --- 双面渲染 ---
    bool            doubleSided = false;

    // --- 纹理索引 (0xFFFF = 无纹理) ---
    uint16_t        albedoTexture           = 0xFFFF;
    uint16_t        normalTexture           = 0xFFFF;
    uint16_t        metallicRoughnessTexture = 0xFFFF;
    uint16_t        emissiveTexture         = 0xFFFF;
    uint16_t        aoTexture               = 0xFFFF;

    // --- 便捷工厂 ---

    static Material defaultPBR() {
        Material m;
        m.type = MaterialType::PBR;
        m.name = "DefaultPBR";
        return m;
    }

    static Material defaultPhong() {
        Material m;
        m.type = MaterialType::BlinnPhong;
        m.name = "DefaultPhong";
        return m;
    }

    static Material unlit(const Vec3& color) {
        Material m;
        m.type = MaterialType::Unlit;
        m.baseColor = color;
        m.name = "Unlit";
        return m;
    }

    static Material transparent(const Vec3& color, double alpha) {
        Material m;
        m.type = MaterialType::PBR;
        m.alphaMode = AlphaMode::Blend;
        m.baseColor = color;
        m.alpha = alpha;
        m.name = "Transparent";
        return m;
    }

    /// 是否半透明
    bool isTransparent() const {
        return alphaMode == AlphaMode::Blend
            || (alphaMode == AlphaMode::Opaque && alpha < 1.0 - 1e-6);
    }

    /// 是否有纹理
    bool hasTextures() const {
        return albedoTexture != 0xFFFF
            || normalTexture != 0xFFFF
            || metallicRoughnessTexture != 0xFFFF;
    }
};

// ============================================================
// GPU 端材质常量布局 (std140 / std430 compatible)
//
// 与 shader 中 MaterialUBO 1:1 对应
// ============================================================

struct alignas(16) MaterialGPU {
    // --- vec4[0]: baseColor.rgb + metallic ---
    float baseColor[3];
    float metallic;

    // --- vec4[1]: emissive.rgb + roughness ---
    float emissive[3];
    float roughness;

    // --- vec4[2]: specular.rgb + shininess ---
    float specular[3];
    float shininess;

    // --- vec4[3]: 标志位 ---
    float alpha;
    float ao;
    float emissiveStrength;
    float alphaCutoff;

    // --- vec4[4]: type + alphaMode + texture flags ---
    uint32_t materialType;      // MaterialType 枚举
    uint32_t alphaMode;         // AlphaMode 枚举
    uint32_t textureFlags;      // 位掩码: bit0=albedo, bit1=normal, bit2=mr, bit3=emissive, bit4=ao
    uint32_t doubleSided;       // 0 或 1

    /// 从 CPU Material 转换
    static MaterialGPU fromMaterial(const Material& m) {
        MaterialGPU g{};
        g.baseColor[0] = static_cast<float>(m.baseColor.x);
        g.baseColor[1] = static_cast<float>(m.baseColor.y);
        g.baseColor[2] = static_cast<float>(m.baseColor.z);
        g.metallic     = static_cast<float>(std::clamp(m.metallic, 0.0, 1.0));

        g.emissive[0]  = static_cast<float>(m.emissive.x * m.emissiveStrength);
        g.emissive[1]  = static_cast<float>(m.emissive.y * m.emissiveStrength);
        g.emissive[2]  = static_cast<float>(m.emissive.z * m.emissiveStrength);
        g.roughness    = static_cast<float>(std::clamp(m.roughness, 0.04, 1.0));

        g.specular[0]  = static_cast<float>(m.specular.x);
        g.specular[1]  = static_cast<float>(m.specular.y);
        g.specular[2]  = static_cast<float>(m.specular.z);
        g.shininess    = static_cast<float>(m.shininess);

        g.alpha             = static_cast<float>(m.alpha);
        g.ao                = static_cast<float>(m.ao);
        g.emissiveStrength  = static_cast<float>(m.emissiveStrength);
        g.alphaCutoff       = static_cast<float>(m.alphaCutoff);

        g.materialType  = static_cast<uint32_t>(m.type);
        g.alphaMode     = static_cast<uint32_t>(m.alphaMode);
        g.textureFlags  = 0;
        if (m.albedoTexture            != 0xFFFF) g.textureFlags |= (1u << 0);
        if (m.normalTexture            != 0xFFFF) g.textureFlags |= (1u << 1);
        if (m.metallicRoughnessTexture != 0xFFFF) g.textureFlags |= (1u << 2);
        if (m.emissiveTexture          != 0xFFFF) g.textureFlags |= (1u << 3);
        if (m.aoTexture                != 0xFFFF) g.textureFlags |= (1u << 4);
        g.doubleSided   = m.doubleSided ? 1u : 0u;

        return g;
    }
};

static_assert(sizeof(MaterialGPU) == 80, "MaterialGPU must be 80 bytes (5 x vec4)");

} // namespace MulanGeo::Engine
