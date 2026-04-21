#include "SceneRenderer.h"

namespace MulanGeo::Engine {

SceneRenderer::SceneRenderer(RHIDevice* device)
    : m_device(device), m_cache(device) {}

void SceneRenderer::render(const RenderQueue& queue, const Camera& camera, CommandList* cmdList) {
    m_stats = {};

    // 1. 选择 PSO
    PipelineState* pso = selectPipeline();
    if (!pso) return;
    cmdList->setPipelineState(pso);

    // 2. 设置视口
    Viewport vp{0.0f, 0.0f,
                 static_cast<float>(camera.width()),
                 static_cast<float>(camera.height()),
                 0.0f, 1.0f};
    cmdList->setViewport(vp);

    // 3. 遍历 RenderQueue 绘制（每个 drawItem 绑定 UBO）
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

    // 更新 Object UBO（每 draw call 一次）
    PipelineState* pso = selectPipeline();
    if (m_objectBuffer && m_device && pso) {
        struct alignas(16) ObjUBO {
            float world[16];
            float normalMat[9];
            float _pad1[3];
            uint32_t pickId;
            float _pad2[3];
        };

        ObjUBO obj{};
        for (int i = 0; i < 16; ++i)
            obj.world[i] = static_cast<float>(glm::value_ptr(item.worldTransform)[i]);

        Mat3 normalMat3 = glm::transpose(glm::inverse(Mat3(item.worldTransform)));
        for (int col = 0; col < 3; ++col)
            for (int row = 0; row < 3; ++row)
                obj.normalMat[col * 3 + row] = static_cast<float>(normalMat3[col][row]);

        obj.pickId = item.pickId;
        m_objectBuffer->update(0, sizeof(ObjUBO), &obj);

        // 选中面：覆盖材质为高亮色
        if (item.selected && m_materialBuffer) {
            struct alignas(16) HighlightMat {
                float baseColor[3]; float _p0;
                float lightDir[3];  float _p1;
                float ambientColor[3]; float _p2;
                float wireColor[3]; float _p3;
            };
            HighlightMat hl{};
            hl.baseColor[0] = 0.3f; hl.baseColor[1] = 0.6f; hl.baseColor[2] = 1.0f; // 高亮蓝
            hl.lightDir[0]  = -0.3f; hl.lightDir[1]  = -1.0f; hl.lightDir[2]  = -0.4f;
            hl.ambientColor[0] = 0.2f; hl.ambientColor[1] = 0.2f; hl.ambientColor[2] = 0.2f;
            hl.wireColor[0] = 0.3f; hl.wireColor[1] = 0.6f; hl.wireColor[2] = 1.0f;
            m_materialBuffer->update(0, sizeof(HighlightMat), &hl);
        }

        RHIDevice::UniformBufferBind uboBinds[] = {
            { 0, m_cameraBuffer,   0, m_cameraBuffer->desc().size },
            { 1, m_objectBuffer,   0, m_objectBuffer->desc().size },
            { 2, m_materialBuffer, 0, m_materialBuffer->desc().size },
        };
        m_device->bindUniformBuffers(cmdList, pso, uboBinds, 3);

        // 选中面画完后恢复默认材质
        if (item.selected && m_materialBuffer) {
            // 下一个 item 的 drawItem 会重新绑定材质
        }
    }

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
