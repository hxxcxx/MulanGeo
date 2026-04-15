/*
 * Wireframe 渲染 — 像素着色器
 * 纯色输出
 */

#include "Common.hlsli"

float4 main(VS_OUTPUT_SIMPLE input) : SV_TARGET {
    return float4(WireColor, 1.0);
}
