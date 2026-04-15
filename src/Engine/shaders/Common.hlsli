/*
 * Shader 公共头 — 所有 shader 共享的常量与结构体
 *
 * Camera (b0): 视图/投影矩阵
 * Object (b1): 每物体世界变换、pickId
 * Material (b2): 材质参数
 */

#ifndef MULAN_GEO_COMMON_HLSLI
#define MULAN_GEO_COMMON_HLSLI

// ============================================================
// 相机常量（每帧更新一次）
// ============================================================
cbuffer Camera : register(b0) {
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float3   CameraPos;
    float    _pad0;
};

// ============================================================
// 物体常量（每 draw call 更新）
// ============================================================
cbuffer Object : register(b1) {
    float4x4 World;
    float3x3 NormalMatrix;
    uint     PickId;
    float    _pad1;
};

// ============================================================
// 材质常量（按渲染模式）
// ============================================================
cbuffer Material : register(b2) {
    float3 BaseColor;
    float  _pad2;
    float3 LightDir;       // 归一化定向光方向
    float  _pad3;
    float3 AmbientColor;
    float  _pad4;
    float3 WireColor;
    float  _pad5;
};

// ============================================================
// 顶点输入 / 输出结构体
// ============================================================

// CAD 标准顶点: pos(3f) + normal(3f) + uv(2f) = 32 bytes
struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
};

// 仅有位置的顶点（wireframe / pick）
struct VS_INPUT_POS {
    float3 position : POSITION;
};

// 顶点着色器输出
struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
};

// wireframe / pick 用的简化输出
struct VS_OUTPUT_SIMPLE {
    float4 position : SV_POSITION;
};

#endif // MULAN_GEO_COMMON_HLSLI
