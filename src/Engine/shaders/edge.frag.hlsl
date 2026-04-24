/*
 * Edge 渲染 — 像素着色器
 * 输出 WireColor 作为边线颜色（深灰/黑色）
 */

#include "Common.hlsli"

float4 main(VS_OUTPUT_SIMPLE input) : SV_TARGET {
    return float4(WireColor, 1.0);
}
