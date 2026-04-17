/**
 * @file PipelineState.h
 * @brief 渲染管线状态集与接口定义
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "RenderState.h"
#include "Shader.h"
#include "VertexLayout.h"

#include <cstdint>
#include <string_view>

namespace MulanGeo::Engine {

// ============================================================
// 输入布局元素（PSO 层定义 stride）
// ============================================================

struct InputLayoutElement {
    uint32_t    inputSlot     = 0;   // vertex buffer slot
    uint32_t    stride        = 0;   // 该 slot 的顶点 stride
    uint32_t    instanceDataStepRate = 0;  // 0 = per-vertex
};

// ============================================================
// 图形管线描述结构体
// ============================================================

struct GraphicsPipelineDesc {
    std::string_view name;

    // 着色器
    Shader* vs = nullptr;   // Vertex
    Shader* ps = nullptr;   // Pixel / Fragment
    Shader* gs = nullptr;   // Geometry（可选）
    Shader* cs = nullptr;   // Compute（仅计算管线）

    // 输入布局 — 关联到 VertexLayout
    VertexLayout        vertexLayout;
    PrimitiveTopology   topology   = PrimitiveTopology::TriangleList;

    // 光栅化
    CullMode   cullMode   = CullMode::Back;
    FrontFace  frontFace  = FrontFace::CounterClockwise;
    FillMode   fillMode   = FillMode::Solid;

    // 深度/模板
    DepthStencilDesc depthStencil;

    // 混合
    BlendDesc blend;

    // 渲染目标格式（用于创建时验证）
    static constexpr uint8_t kMaxRenderTargets = 8;
    // TextureFormat colorFormats[kMaxRenderTargets];  // 后续需要时再加
};
// ============================================================
// 管线状态基类
//
// 涵盖一次绘制所需的全部粗粒度状态。
// 由 Device 创建，通过 CommandList::setPipelineState() 绑定。
// ============================================================

class SwapChain;
class RenderTarget;

class PipelineState {
public:
    virtual ~PipelineState() = default;

    virtual const GraphicsPipelineDesc& desc() const = 0;

    /// 在 SwapChain 就绪后完成管线构建（VK 需要 renderPass，GL 可 noop）
    virtual void finalize(SwapChain* swapchain) = 0;

    /// 在 RenderTarget 就绪后完成管线构建（离屏渲染）
    virtual void finalize(RenderTarget* rt) { (void)rt; }

    // 便捷查询
    PrimitiveTopology topology() const { return desc().topology; }

protected:
    PipelineState() = default;
    PipelineState(const PipelineState&) = delete;
    PipelineState& operator=(const PipelineState&) = delete;
};

} // namespace MulanGeo::Engine
