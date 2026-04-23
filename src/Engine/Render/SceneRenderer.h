/**
 * @file SceneRenderer.h
 * @brief 场景渲染器 — 管理管线资源（Shader/PSO/UBO），录制 RHI 绘制命令
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/Shader.h"
#include "../RHI/PipelineState.h"
#include "../RHI/Buffer.h"
#include "../RHI/VertexLayout.h"
#include "../RHI/SwapChain.h"
#include "../RHI/RenderTarget.h"
#include "../Scene/Camera.h"
#include "RenderGeometry.h"
#include "GeometryCache.h"

#include <cstdint>

namespace MulanGeo::Engine {

// ============================================================
// 渲染模式
// ============================================================

enum class RenderMode : uint8_t {
    Solid,       // 实体填充
    Wireframe,   // 线框
    SolidWire,   // 实体 + 线框叠加
};

// ============================================================
// 帧统计
// ============================================================

struct RenderStats {
    uint32_t drawCalls   = 0;
    uint32_t triangles   = 0;
    uint32_t items       = 0;
};

// ============================================================
// 场景渲染器
// ============================================================

class SceneRenderer {
public:
    explicit SceneRenderer(RHIDevice* device);

    // --- 初始化（创建 Shader / PSO / UBO）---
    bool init();
    void cleanup();

    /// PSO finalize（在 swapchain/rendertarget 就绪后调用）
    void finalizePipeline(SwapChain* swapchain);
    void finalizePipeline(RenderTarget* rt);

    // --- 渲染模式 ---
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode renderMode() const { return m_renderMode; }

    // --- 清理 GPU 缓存 ---
    void clearCache() { m_cache.clear(); }

    // --- 渲染 ---
    void render(const RenderQueue& queue, const Camera& camera, CommandList* cmdList);

    // --- 统计 ---
    const RenderStats& stats() const { return m_stats; }

private:
    void loadShaders();
    void createPSOs();
    void createUBOs();
    void updateCameraUBO(const Camera& camera);
    PipelineState* selectPipeline() const;
    void drawItem(const RenderItem& item, CommandList* cmdList);

    RHIDevice*   m_device;
    GeometryCache m_cache;

    // --- Shader / PSO ---
    ResourcePtr<Shader>         m_solidVs;
    ResourcePtr<Shader>         m_solidFs;
    ResourcePtr<PipelineState>  m_solidPso;
    VertexLayout                m_vertexLayout;

    // --- UBO ---
    ResourcePtr<Buffer>         m_cameraBuffer;
    ResourcePtr<Buffer>         m_objectBuffer;
    ResourcePtr<Buffer>         m_materialBuffer;

    RenderMode   m_renderMode = RenderMode::Solid;
    RenderStats  m_stats;
};

} // namespace MulanGeo::Engine
