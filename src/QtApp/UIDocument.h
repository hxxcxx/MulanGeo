/**
 * @file UIDocument.h
 * @brief UI 文档 — 桥接 Document 数据层与 EngineView 渲染层
 * @author hxxcxx
 * @date 2026-04-22
 *
 * 职责：
 *  - 持有 Document（数据）
 *  - 通过 collector 回调将 Entity 的 displayMesh() 喂给 EngineView
 *  - 管理视图绑定（attach/detach）
 *
 * 位于 QtApp 层，是唯一同时依赖 Document 模块和 Engine 模块的类。
 */
#pragma once

#include <MulanGeo/Document/Document.h>
#include <MulanGeo/Engine/Render/RenderGeometry.h>
#include <MulanGeo/Engine/Scene/Camera.h>
#include <MulanGeo/Engine/Math/Math.h>
#include <MulanGeo/Engine/Math/AABB.h>

#include <deque>
#include <memory>

namespace MulanGeo::Engine {
class EngineView;
}

class UIDocument {
public:
    explicit UIDocument(MulanGeo::Document::Document* doc);
    ~UIDocument();

    // --- 数据层 ---

    MulanGeo::Document::Document& document() { return *m_doc; }
    const MulanGeo::Document::Document& document() const { return *m_doc; }

    // --- 视图连接 ---

    /// 绑定 EngineView（设置 collector 回调 + 适配相机）
    void attachView(MulanGeo::Engine::EngineView* view);

    /// 解除绑定
    void detachView();

    MulanGeo::Engine::EngineView* view() const { return m_view; }

private:
    /// 计算所有 Entity 的总包围盒
    MulanGeo::Engine::AABB computeSceneBounds();

    MulanGeo::Document::Document*  m_doc;   // 不拥有，由 DocumentManager 管理
    MulanGeo::Engine::EngineView*  m_view = nullptr;

    /// collector 持有的渲染几何缓存（每帧重建）
    std::deque<MulanGeo::Engine::RenderGeometry> m_geometries;
};
