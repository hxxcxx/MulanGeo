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
        m_geometries.clear();
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
            Engine::RenderGeometry geo{};
            geo.vertexBytes  = std::as_bytes(std::span{part->vertices});
            geo.indexBytes   = std::as_bytes(std::span{part->indices});
            geo.vertexCount  = static_cast<uint32_t>(part->vertices.size());
            geo.indexCount   = static_cast<uint32_t>(part->indices.size());
            geo.vertexStride = sizeof(float) * 8; // pos(3) + normal(3) + uv(2)
            geo.topology     = Engine::PrimitiveTopology::TriangleList;
            m_geometries.push_back(geo);

            Engine::RenderItem item;
            item.geometry       = &m_geometries.back();
            item.worldTransform = node.worldTransform();
            item.pickId         = node.pickId();

            queue.add(item);
        });
    }

private:
    std::vector<Engine::RenderGeometry> m_geometries;
};

} // namespace MulanGeo::IO
