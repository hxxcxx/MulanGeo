/**
 * @file UIDocument.cpp
 * @brief UIDocument 实现 — 遍历 Document Entity 填充 RenderQueue
 * @author hxxcxx
 * @date 2026-04-22
 */
#include "UIDocument.h"

#include <MulanGeo/Engine/Render/EngineView.h>
#include <MulanGeo/Engine/Geometry/MeshGeometry.h>
#include <MulanGeo/Document/Entity.h>
#include <MulanGeo/Document/Geometry.h>

UIDocument::UIDocument(MulanGeo::Document::Document* doc)
    : m_doc(doc)
{}

UIDocument::~UIDocument() {
    detachView();
}

void UIDocument::attachView(MulanGeo::Engine::EngineView* view) {
    if (m_view) detachView();
    m_view = view;

    view->setCollector([this](const MulanGeo::Engine::Camera& cam,
                              MulanGeo::Engine::RenderQueue& queue) {
        queue.clear();
        m_geometries.clear();

        auto frustum = cam.frustum();

        m_doc->forEachEntity([&](MulanGeo::Document::Entity& entity) {
            if (!entity.visible() || !entity.hasGeometry()) return;

            auto* geo = entity.geometry();
            auto bounds = geo->boundingBox();
            if (!bounds.isEmpty() && !frustum.intersects(bounds)) return;

            const auto* mesh = geo->displayMesh();
            if (!mesh || mesh->empty()) return;

            m_geometries.push_back(mesh->asRenderGeometry());

            MulanGeo::Engine::RenderItem item;
            item.geometry       = &m_geometries.back();
            item.worldTransform = entity.localTransform();
            item.pickId         = static_cast<uint32_t>(entity.id().value);
            item.selected       = false;

            queue.add(item);
        });
    });

    auto bounds = computeSceneBounds();
    if (!bounds.isEmpty()) {
        view->camera().fitToBox(bounds);
    }
}

void UIDocument::detachView() {
    if (m_view) {
        m_view->clearCollector();
        m_view = nullptr;
    }
}

MulanGeo::Engine::AABB UIDocument::computeSceneBounds() {
    MulanGeo::Engine::AABB bounds;
    m_doc->forEachEntity([&](const MulanGeo::Document::Entity& entity) {
        if (entity.hasGeometry()) {
            auto entityBounds = entity.geometry()->boundingBox();
            if (!entityBounds.isEmpty()) {
                bounds.expand(entityBounds.min);
                bounds.expand(entityBounds.max);
            }
        }
    });
    return bounds;
}
