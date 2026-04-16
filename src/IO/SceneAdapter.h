/**
 * @file SceneAdapter.h
 * @brief 场景适配器，IO层与Engine渲染层的桥接
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "MeshNode.h"
#include "MeshData.h"

#include "MulanGeo/Engine/Scene/Scene.h"
#include "MulanGeo/Engine/Scene/Camera.h"
#include "MulanGeo/Engine/Render/RenderGeometry.h"

namespace MulanGeo::IO {

class SceneAdapter {
public:
    // 从场景树收集可见的 RenderItem 到队列
    // scene:    场景树
    // camera:   用于视锥体裁剪
    // queue:    输出的渲染队列
    void collect(Engine::Scene& scene, const Engine::Camera& camera,
                 Engine::RenderQueue& queue)
    {
        queue.clear();
        auto frustum = camera.frustum();

        scene.traverseVisible([&](Engine::SceneNode& node) {
            auto* meshNode = node.as<MeshNode>();
            if (!meshNode) return;

            const auto* part = meshNode->meshData();
            if (!part || part->vertices.empty()) return;

            // 视锥体裁剪
            const auto& bounds = node.boundingBox();
            if (!bounds.isEmpty() && !frustum.intersects(bounds)) return;

            // IO::MeshPart → Engine::RenderGeometry
            // 零拷贝：直接指向 IO 层的数据
            m_geometry = {};
            m_geometry.vertexBytes  = std::as_bytes(std::span{part->vertices});
            m_geometry.indexBytes   = std::as_bytes(std::span{part->indices});
            m_geometry.vertexCount  = static_cast<uint32_t>(part->vertices.size());
            m_geometry.indexCount   = static_cast<uint32_t>(part->indices.size());
            m_geometry.vertexStride = sizeof(float) * 8; // pos(3) + normal(3) + uv(2)
            // TODO: 根据 Engine 需要的布局选择，当前 IO 的 Vertex 布局直接匹配
            m_geometry.topology     = Engine::PrimitiveTopology::TriangleList;

            Engine::RenderItem item;
            item.geometry       = &m_geometry;
            item.worldTransform = node.worldTransform();
            item.pickId         = node.pickId();

            queue.add(item);
        });
    }

private:
    Engine::RenderGeometry m_geometry;  // 复用，避免每帧分配
};

} // namespace MulanGeo::IO
