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
    explicit SceneRenderer(RHIDevice* device)
        : m_device(device), m_cache(device) {}

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
    void render(const RenderQueue& queue, const Camera& camera, CommandList* cmdList) {
        m_stats = {};

        // 1. 设置视口
        Viewport vp{0.0f, 0.0f,
                     static_cast<float>(camera.width()),
                     static_cast<float>(camera.height()),
                     0.0f, 1.0f};
        cmdList->setViewport(vp);

        // 2. 清屏
        cmdList->clearColor(0.15f, 0.15f, 0.15f, 1.0f);
        cmdList->clearDepth(1.0f);

        // 3. 选择 PSO
        PipelineState* pso = selectPipeline();
        if (!pso) return;
        cmdList->setPipelineState(pso);

        // 4. 遍历 RenderQueue 绘制
        for (const auto& item : queue.items()) {
            drawItem(item, cmdList);
        }
    }

    // --- 统计 ---
    const RenderStats& stats() const { return m_stats; }

private:
    PipelineState* selectPipeline() const {
        switch (m_renderMode) {
            case RenderMode::Solid:
            case RenderMode::SolidWire:
                return m_solidPso;
            case RenderMode::Wireframe:
                return m_wirePso;
            case RenderMode::Pick:
                return m_pickPso;
            default:
                return m_solidPso;
        }
    }

    void drawItem(const RenderItem& item, CommandList* cmdList) {
        if (!item.geometry) return;

        const auto* geo = item.geometry;

        // 获取或上传 GPU 缓冲区
        const auto* gpu = m_cache.getOrUpload(geo);

        if (!gpu || !gpu->vertexBuffer) return;

        // 绑定顶点缓冲区
        cmdList->setVertexBuffer(0, gpu->vertexBuffer);

        // 绘制
        if (gpu->indexBuffer && gpu->indexCount > 0) {
            cmdList->setIndexBuffer(gpu->indexBuffer);
            cmdList->drawIndexed(DrawIndexedAttribs{
                .indexCount = gpu->indexCount,
            });
            m_stats.triangles += gpu->indexCount / 3;
        } else {
            cmdList->draw(DrawAttribs{
                .vertexCount = gpu->vertexCount,
            });
            m_stats.triangles += gpu->vertexCount / 3;
        }

        ++m_stats.drawCalls;
        ++m_stats.items;
    }

    RHIDevice*   m_device;
    GeometryCache m_cache;

    RenderMode   m_renderMode = RenderMode::Solid;
    RenderStats  m_stats;

    PipelineState* m_solidPso = nullptr;
    PipelineState* m_wirePso  = nullptr;
    PipelineState* m_pickPso  = nullptr;
};

} // namespace MulanGeo::Engine
