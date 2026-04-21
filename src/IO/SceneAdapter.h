/**
 * @file SceneAdapter.h
 * @brief 场景适配器，遍历场景树按面收集 RenderItem
 * @author hxxcxx
 * @date 2026-04-21
 */

#pragma once

#include "MulanGeo/Engine/Scene/Scene.h"
#include "MulanGeo/Engine/Scene/Camera.h"
#include "MulanGeo/Engine/Scene/GeometryNode.h"
#include "MulanGeo/Engine/Render/RenderGeometry.h"

namespace MulanGeo::IO {

class SceneAdapter {
public:
    void collect(Engine::Scene& scene, const Engine::Camera& camera,
                 Engine::RenderQueue& queue)
    {
        queue.clear();
        m_geometries.clear();
        auto frustum = camera.frustum();

        scene.traverseVisible([&](Engine::SceneNode& node) {
            auto* geoNode = node.as<Engine::GeometryNode>();
            if (!geoNode) return;

            // 视锥体裁剪（节点级别包围盒）
            const auto& bounds = node.boundingBox();
            if (!bounds.isEmpty() && !frustum.intersects(bounds)) return;

            // 按面收集 RenderItem
            for (const auto& face : geoNode->faces()) {
                if (!face.mesh || face.mesh->empty()) continue;

                m_geometries.push_back(face.mesh->asRenderGeometry());

                Engine::RenderItem item;
                item.geometry       = &m_geometries.back();
                item.worldTransform = node.worldTransform();
                item.pickId         = face.pickId;
                item.selected       = face.selected;

                queue.add(item);
            }
        });
    }

private:
    std::vector<Engine::RenderGeometry> m_geometries;
};

} // namespace MulanGeo::IO
