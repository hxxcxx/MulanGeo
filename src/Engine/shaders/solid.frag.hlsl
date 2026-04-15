/*
 * Solid 渲染 — 像素着色器
 * 简单定向光 + 环境光
 */

#include "Common.hlsli"

float4 main(VS_OUTPUT input) : SV_TARGET {
    // Lambertian 漫反射
    float3 N = normalize(input.normal);
    float3 L = normalize(-LightDir);    // LightDir 是光方向，取反得到朝光方向
    float  NdotL = max(dot(N, L), 0.0);

    float3 diffuse  = BaseColor * NdotL;
    float3 ambient  = BaseColor * AmbientColor;
    float3 color    = ambient + diffuse;

    // 简单的半球环境光补偿（底部稍暗）
    float hemi = 0.5 + 0.5 * N.y;
    color *= hemi;

    return float4(color, 1.0);
}
