/**
 * @file UIDocument.cpp
 * @brief UIDocument 实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#include "MulanGeo/IO/UIDocument.h"
#include "MulanGeo/IO/MeshDocument.h"
#include "MulanGeo/IO/MeshNode.h"

#include "MulanGeo/Engine/Render/EngineView.h"

namespace MulanGeo::IO {

UIDocument::UIDocument(std::unique_ptr<Document> doc)
    : m_doc(std::move(doc))
{}

UIDocument::~UIDocument() {
    detachView();
}

void UIDocument::rebuildScene() {
    m_scene.clear();

    auto* meshDoc = dynamic_cast<MeshDocument*>(m_doc.get());
    if (!meshDoc) return;

    uint32_t pickId = 1;
    for (const auto& mesh : meshDoc->importData().meshes) {
        auto node = std::make_unique<MeshNode>(mesh.name, pickId++);
        node->setMeshData(&mesh);

        // 计算包围盒
        Engine::AABB bounds;
        for (const auto& v : mesh.vertices) {
            bounds.expand(Engine::Vec3{v.position.x, v.position.y, v.position.z});
        }
        node->setBoundingBox(bounds);

        m_scene.addChild(std::move(node));
    }
    m_scene.updateWorldTransforms();
}

void UIDocument::attachView(Engine::EngineView* view) {
    if (m_view) detachView();
    m_view = view;

    // 设置收集回调：每帧渲染前由 EngineView 调用
    view->setCollector([this](const Engine::Camera& cam, Engine::RenderQueue& queue) {
        m_adapter.collect(m_scene, cam, queue);
    });

    // 适配相机到场景包围盒
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

Engine::AABB UIDocument::computeSceneBounds() {
    Engine::AABB bounds;
    m_scene.traverse([&](Engine::SceneNode& node) {
        const auto& nodeBounds = node.boundingBox();
        if (!nodeBounds.isEmpty()) {
            bounds.expand(nodeBounds.min);
            bounds.expand(nodeBounds.max);
        }
    });
    return bounds;
}

} // namespace MulanGeo::IO
