/**
 * @file PipelineState.h
 * @brief 渲染管线状态集与接口定义
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "Shader.h"
#include "VertexLayout.h"

#include <cstdint>
#include <string_view>

namespace MulanGeo::Engine {

// ============================================================
// 图元拓扑
// ============================================================

enum class PrimitiveTopology : uint8_t {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,

    // 带邻接信息（用于 geometry shader）
    LineListAdj,
    LineStripAdj,
    TriangleListAdj,
    TriangleStripAdj,
};

// ============================================================
// 光栅化状态
// ============================================================

enum class CullMode : uint8_t {
    None,    // 双面渲染
    Front,
    Back,
};

enum class FrontFace : uint8_t {
    CounterClockwise,
    Clockwise,
};

enum class FillMode : uint8_t {
    Solid,
    Wireframe,
};

// ============================================================
// 深度/模板状态
// ============================================================

enum class CompareFunc : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

enum class StencilOp : uint8_t {
    Keep,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

struct DepthStencilOpDesc {
    StencilOp  failOp      = StencilOp::Keep;
    StencilOp  depthFailOp = StencilOp::Keep;
    StencilOp  passOp      = StencilOp::Keep;
    CompareFunc func       = CompareFunc::Always;
};

struct DepthStencilDesc {
    bool             depthEnable   = true;
    bool             depthWrite    = true;
    CompareFunc      depthFunc     = CompareFunc::LessEqual;
    bool             stencilEnable = false;
    uint8_t          stencilReadMask  = 0xFF;
    uint8_t          stencilWriteMask = 0xFF;
    DepthStencilOpDesc frontFace;
    DepthStencilOpDesc backFace;
};

// ============================================================
// 混合状态
// ============================================================

enum class BlendFactor : uint8_t {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha,
    DstColor,
    InvDstColor,
};

enum class BlendOp : uint8_t {
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,
};

struct RenderTargetBlendDesc {
    bool       blendEnable   = false;
    BlendFactor srcBlend     = BlendFactor::One;
    BlendFactor dstBlend     = BlendFactor::Zero;
    BlendOp     blendOp      = BlendOp::Add;
    BlendFactor srcBlendAlpha = BlendFactor::One;
    BlendFactor dstBlendAlpha = BlendFactor::Zero;
    BlendOp     blendOpAlpha = BlendOp::Add;
    uint8_t     writeMask    = 0x0F;  // RGBA
};

struct BlendDesc {
    bool                   alphaToCoverage = false;
    bool                   independentBlend = false;
    RenderTargetBlendDesc  renderTargets[8];
};

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

class PipelineState {
public:
    virtual ~PipelineState() = default;

    virtual const GraphicsPipelineDesc& desc() const = 0;

    /// 在 SwapChain 就绪后完成管线构建（VK 需要 renderPass，GL 可 noop）
    virtual void finalize(SwapChain* swapchain) = 0;

    // 便捷查询
    PrimitiveTopology topology() const { return desc().topology; }

protected:
    PipelineState() = default;
    PipelineState(const PipelineState&) = delete;
    PipelineState& operator=(const PipelineState&) = delete;
};

} // namespace MulanGeo::Engine
