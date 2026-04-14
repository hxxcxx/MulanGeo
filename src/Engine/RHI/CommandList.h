/*
 * 命令列表 — GPU 命令录制接口
 *
 * 定义 CommandList 基类，负责所有渲染命令的录制：
 * 设置管线状态、绑定缓冲区、绘制、资源屏障等。
 *
 * 设计原则：
 * - Buffer/Texture 不提供 bind()，由 CommandList 统一绑定
 * - 支持多后端（GL/VK/D3D12）各自实现
 * - 命令录制与提交分离
 */

#pragma once

#include "Buffer.h"
#include "VertexLayout.h"

#include <cstdint>

namespace MulanGeo::Engine {

// ============================================================
// 前向声明
// ============================================================

class PipelineState;
class Shader;

// ============================================================
// 视口与裁剪矩形
// ============================================================

struct Viewport {
    float x         = 0.0f;
    float y         = 0.0f;
    float width     = 0.0f;
    float height    = 0.0f;
    float minDepth  = 0.0f;
    float maxDepth  = 1.0f;
};

struct ScissorRect {
    int32_t x      = 0;
    int32_t y      = 0;
    int32_t width  = 0;
    int32_t height = 0;
};

// ============================================================
// 绘制参数
// ============================================================

struct DrawAttribs {
    uint32_t vertexCount   = 0;
    uint32_t instanceCount = 1;
    uint32_t startVertex   = 0;
    uint32_t startInstance = 0;
};

struct DrawIndexedAttribs {
    uint32_t indexCount    = 0;
    uint32_t instanceCount = 1;
    uint32_t startIndex    = 0;
    int32_t  baseVertex    = 0;
    uint32_t startInstance = 0;
    IndexType indexType    = IndexType::UInt32;
};

// ============================================================
// 资源状态转换（VK/D3D12 需要，GL 可忽略）
// ============================================================

enum class ResourceState : uint32_t {
    Common          = 0,
    VertexBuffer    = 1,
    IndexBuffer     = 2,
    UniformBuffer   = 3,
    ShaderResource  = 4,
    UnorderedAccess = 5,
    RenderTarget    = 6,
    DepthWrite      = 7,
    DepthRead       = 8,
    Present         = 9,
    CopyDest        = 10,
    CopySrc         = 11,
};

enum class ResourceTransitionMode : uint8_t {
    None,        // 不转换（GL 后端常用）
    Transition,  // 自动转换
};

// ============================================================
// 命令列表基类
// ============================================================

class CommandList {
public:
    virtual ~CommandList() = default;

    // --- 生命周期 ---

    virtual void begin() = 0;
    virtual void end()   = 0;

    // --- 管线状态 ---

    virtual void setPipelineState(PipelineState* pso) = 0;

    // --- 视口 / 裁剪 ---

    virtual void setViewport(const Viewport& vp) = 0;
    virtual void setScissorRect(const ScissorRect& rect) = 0;

    // --- 缓冲区绑定（核心：Buffer 不自带 bind，由 CommandList 统一管理）---

    virtual void setVertexBuffer(uint32_t slot, Buffer* buffer,
                                 uint32_t offset = 0) = 0;
    virtual void setVertexBuffers(uint32_t startSlot, uint32_t count,
                                  Buffer** buffers, uint32_t* offsets) = 0;
    virtual void setIndexBuffer(Buffer* buffer, uint32_t offset = 0,
                                IndexType type = IndexType::UInt32) = 0;

    // --- 绘制 ---

    virtual void draw(const DrawAttribs& attribs) = 0;
    virtual void drawIndexed(const DrawIndexedAttribs& attribs) = 0;

    // --- 资源更新 ---

    virtual void updateBuffer(Buffer* buffer, uint32_t offset,
                              uint32_t size, const void* data,
                              ResourceTransitionMode mode =
                                  ResourceTransitionMode::Transition) = 0;

    // --- 资源状态转换 ---

    virtual void transitionResource(Buffer* buffer,
                                    ResourceState newState) = 0;

    // --- 清除 ---

    virtual void clearColor(float r, float g, float b, float a) = 0;
    virtual void clearDepth(float depth) = 0;
    virtual void clearStencil(uint8_t stencil) = 0;

protected:
    CommandList() = default;
    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;
};

} // namespace MulanGeo::Engine
