/**
 * @file SceneRenderer.h
 * @brief 场景渲染器，遍历RenderQueue录制RHI绘制命令
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/PipelineState.h"
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
    Pick,        // 拾取模式
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

    // --- 渲染模式 ---
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode renderMode() const { return m_renderMode; }

    // --- 管线状态（外部注入）---
    void setSolidPipeline(PipelineState* pso) { m_solidPso = pso; }
    void setWirePipeline(PipelineState* pso)  { m_wirePso = pso; }
    void setPickPipeline(PipelineState* pso)  { m_pickPso = pso; }

    // --- 清理 GPU 缓存 ---
    void clearCache() { m_cache.clear(); }

    // --- 渲染 ---
    void render(const RenderQueue& queue, const Camera& camera, CommandList* cmdList);

    // --- 统计 ---
    const RenderStats& stats() const { return m_stats; }

private:
    PipelineState* selectPipeline() const;
    void drawItem(const RenderItem& item, CommandList* cmdList);

    RHIDevice*   m_device;
    GeometryCache m_cache;

    RenderMode   m_renderMode = RenderMode::Solid;
    RenderStats  m_stats;

    PipelineState* m_solidPso = nullptr;
    PipelineState* m_wirePso  = nullptr;
    PipelineState* m_pickPso  = nullptr;
};

} // namespace MulanGeo::Engine
