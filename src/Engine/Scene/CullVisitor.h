/**
 * @file CullVisitor.h
 * @brief 视锥裁剪访问器 — 遍历场景树，收集可见的 GeometryNode 到 RenderQueue
 * @author hxxcxx
 * @date 2026-04-23
 *
 * 职责：
 *  - 遍历 Scene 树（跳过不可见子树）
 *  - 对 GeometryNode 做视锥裁剪
 *  - 通过裁剪的节点生成 RenderItem 加入 RenderQueue
 *
 * 参考 other/ 的 CPMGsCullVisitor 设计。
 */
#pragma once

#include "SceneNode.h"
#include "GeometryNode.h"
#include "Frustum.h"
#include "../Render/RenderGeometry.h"
#include "../Scene/Camera.h"

#include <cstdint>

namespace MulanGeo::Engine {

class CullVisitor {
public:
    CullVisitor(const Frustum& frustum, RenderQueue& queue)
        : m_frustum(frustum), m_queue(queue) {}

    /// 访问一个节点（在 traverse 回调中使用）
    void visit(SceneNode& node) {
        if (!node.isEffectivelyVisible()) return;

        auto* geoNode = node.as<GeometryNode>();
        if (!geoNode) return;

        // 视锥裁剪
        const auto& bounds = geoNode->worldBoundingBox();
        if (!bounds.isEmpty() && !m_frustum.intersects(bounds)) return;

        // 面几何 → RenderQueue
        if (geoNode->hasRenderData()) {
            RenderItem item;
            item.geometry       = &geoNode->cachedRenderGeometry();
            item.worldTransform = geoNode->worldTransform();
            item.pickId         = geoNode->pickId();
            item.materialIndex  = geoNode->materialIndex();
            item.selected       = geoNode->selected();

            m_queue.add(item);
        }

        // 边线几何 → RenderQueue（标记 isEdge）
        if (geoNode->hasEdgeData()) {
            RenderItem edgeItem;
            edgeItem.geometry       = &geoNode->cachedEdgeGeometry();
            edgeItem.worldTransform = geoNode->worldTransform();
            edgeItem.pickId         = geoNode->pickId();
            edgeItem.materialIndex  = geoNode->materialIndex();
            edgeItem.selected       = geoNode->selected();
            edgeItem.isEdge         = true;

            m_queue.add(edgeItem);
        }
    }

private:
    const Frustum& m_frustum;
    RenderQueue&   m_queue;
};

} // namespace MulanGeo::Engine
