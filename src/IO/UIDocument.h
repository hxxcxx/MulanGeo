/**
 * @file UIDocument.h
 * @brief UI文档 — 协调数据(Document)、场景(Scene)与视图(EngineView)
 * @author hxxcxx
 * @date 2026-04-18
 *
 * 职责：
 *  - 拥有 Document（纯数据）
 *  - 拥有 Scene（运行时场景树）
 *  - 通过 SceneAdapter 桥接 Scene 与 EngineView 的 RenderQueue
 *  - 管理视图绑定 (attach/detach)
 */
#pragma once

#include "Document.h"
#include "SceneAdapter.h"

#include "MulanGeo/Engine/Scene/Scene.h"
#include "MulanGeo/Engine/Math/AABB.h"

#include <memory>

namespace MulanGeo::Engine {
class EngineView;
}

namespace MulanGeo::IO {

class IO_API UIDocument {
public:
    explicit UIDocument(std::unique_ptr<Document> doc);
    ~UIDocument();

    // --- 数据层 ---

    Document& document() { return *m_doc; }
    const Document& document() const { return *m_doc; }

    // --- 场景层 ---

    Engine::Scene& scene() { return m_scene; }
    const Engine::Scene& scene() const { return m_scene; }

    /// 从 Document 数据重建 Scene 树
    void rebuildScene();

    // --- 视图连接 ---

    /// 绑定 EngineView（设置 collector 回调 + 适配相机）
    void attachView(Engine::EngineView* view);

    /// 解除绑定
    void detachView();

    Engine::EngineView* view() const { return m_view; }

private:
    Engine::AABB computeSceneBounds();

    std::unique_ptr<Document>  m_doc;
    Engine::Scene              m_scene;
    SceneAdapter               m_adapter;
    Engine::EngineView*        m_view = nullptr;
};

} // namespace MulanGeo::IO
