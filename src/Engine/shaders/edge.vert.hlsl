/*
 * Edge 渲染 — 顶点着色器
 * 复用完整顶点布局 (pos+normal+uv)，但只变换位置
 */

#include "Common.hlsli"

VS_OUTPUT_SIMPLE main(VS_INPUT input) {
    VS_OUTPUT_SIMPLE output;
    output.position = mul(ViewProjection, mul(World, float4(input.position, 1.0)));
    return output;
}
