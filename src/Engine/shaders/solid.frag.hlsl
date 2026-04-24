/*
 * Solid 渲染 — 像素着色器
 * 双面 Lambertian 漫反射 + 环境光（参考 OCCT/FreeCAD 风格）
 */

#include "Common.hlsli"

float4 main(VS_OUTPUT input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 L = normalize(-LightDir);

    // 双面光照：取绝对值确保背面也有光
    float NdotL = abs(dot(N, L));

    // 主光漫反射
    float3 diffuse = BaseColor * NdotL;

    // 环境光（较高强度，确保底面不会全黑）
    float3 ambient = BaseColor * AmbientColor;

    // 柔和的明暗渐变（不使用激进半球补偿）
    float gradient = 0.7 + 0.3 * (0.5 + 0.5 * N.y);

    float3 color = (ambient + diffuse) * gradient;

    return float4(color, 1.0);
}
