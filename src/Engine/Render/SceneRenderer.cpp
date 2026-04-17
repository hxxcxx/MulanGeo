#include "SceneRenderer.h"

namespace MulanGeo::Engine {

SceneRenderer::SceneRenderer(RHIDevice* device)
    : m_device(device), m_cache(device) {}

void SceneRenderer::render(const RenderQueue& queue, const Camera& camera, CommandList* cmdList) {
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

PipelineState* SceneRenderer::selectPipeline() const {
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

void SceneRenderer::drawItem(const RenderItem& item, CommandList* cmdList) {
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

} // namespace MulanGeo::Engine
