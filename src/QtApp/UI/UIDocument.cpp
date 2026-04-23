/**
 * @file UIDocument.cpp
 * @brief UIDocument 实现 — 从 Scene 遍历可见节点填充 RenderQueue
 * @author hxxcxx
 * @date 2026-04-23
 */
#include "UIDocument.h"
#include "SceneBuilder.h"

#include <MulanGeo/Engine/Render/EngineView.h>
#include <MulanGeo/Engine/Scene/GeometryNode.h>
#include <MulanGeo/Engine/Scene/Frustum.h>

UIDocument::UIDocument(MulanGeo::Document::Document* doc)
    : m_doc(doc)
{}

UIDocument::~UIDocument() {
    detachView();
}

void UIDocument::attachView(MulanGeo::Engine::EngineView* view) {
    if (m_view) detachView();
    m_view = view;

    // 一次性构建 Scene
    m_scene = SceneBuilder::build(*m_doc);
    m_pickIdMap = SceneBuilder::buildPickIdMap(*m_doc);

    // 设置 collector：从 Scene 遍历可见的 GeometryNode 填充 RenderQueue
    view->setCollector([this](const MulanGeo::Engine::Camera& cam,
                              MulanGeo::Engine::RenderQueue& queue) {
        queue.clear();

        auto frustum = cam.frustum();
        auto camPos = cam.eyePosition();

        m_scene->traverseVisible([&](MulanGeo::Engine::SceneNode& node) {
            // 只处理 GeometryNode
            auto* geoNode = node.as<MulanGeo::Engine::GeometryNode>();
            if (!geoNode) return;
            if (!geoNode->hasRenderData()) return;

            // 视锥裁剪
            const auto& bounds = geoNode->worldBoundingBox();
            if (!bounds.isEmpty() && !frustum.intersects(bounds)) return;

            // 直接使用缓存的 RenderGeometry，不重建
            MulanGeo::Engine::RenderItem item;
            item.geometry       = &geoNode->cachedRenderGeometry();
            item.worldTransform = geoNode->worldTransform();
            item.pickId         = geoNode->pickId();
            item.materialIndex  = geoNode->materialIndex();
            item.selected       = geoNode->selected();

            queue.add(item);
        });
    });

    // 适配相机到场景包围盒
    MulanGeo::Engine::AABB sceneBounds;
    m_scene->traverse([&](MulanGeo::Engine::SceneNode& node) {
        const auto& bounds = node.worldBoundingBox();
        if (!bounds.isEmpty()) {
            sceneBounds.expand(bounds.min);
            sceneBounds.expand(bounds.max);
        }
    });
    if (!sceneBounds.isEmpty()) {
        view->camera().fitToBox(sceneBounds);
    }
}

void UIDocument::detachView() {
    if (m_view) {
        m_view->clearCollector();
        m_view = nullptr;
    }
    m_scene.reset();
    m_pickIdMap.clear();
}

MulanGeo::Document::EntityId UIDocument::resolvePickId(uint32_t pickId) const {
    auto it = m_pickIdMap.find(pickId);
    if (it != m_pickIdMap.end()) return it->second;
    return {};
}
