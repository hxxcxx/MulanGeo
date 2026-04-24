/**
 * @file GeometryNode.h
 * @brief 几何场景节点，持有缓存的渲染几何数据
 * @author hxxcxx
 * @date 2026-04-21
 */

#pragma once

#include "SceneNode.h"
#include "../Geometry/MeshGeometry.h"
#include "../Render/RenderGeometry.h"

#include <cstdint>
#include <vector>

namespace MulanGeo::Engine {

// ============================================================
// 几何节点 — 场景中可渲染的几何体
//
// 持有从 MeshGeometry 缓存的 RenderGeometry，
// 构建（SceneBuilder）时一次性生成，每帧直接使用无需重建。
// ============================================================

class GeometryNode final : public SceneNode {
public:
    static constexpr MulanGeo::NodeType kType = MulanGeo::NodeType::Geometry;

    // --- 面数据 ---

    struct Face {
        MeshGeometry* mesh     = nullptr;  // 不拥有，指向 Document 中的数据
        uint32_t      pickId   = 0;        // 全局唯一拾取 ID
        bool          selected = false;
    };

    explicit GeometryNode(std::string name = {}, uint32_t pickId = 0)
        : SceneNode(kType, std::move(name), pickId) {}

    // --- 缓存的渲染几何（由 SceneBuilder 一次性设置）---

    /// 设置缓存的 RenderGeometry（从 MeshGeometry::asRenderGeometry 生成）
    void setCachedRenderGeometry(const RenderGeometry& geo) { m_cachedGeo = geo; }

    /// 获取缓存的渲染几何（每帧直接使用，不重建）
    const RenderGeometry& cachedRenderGeometry() const { return m_cachedGeo; }

    /// 设置缓存的边线 RenderGeometry
    void setCachedEdgeGeometry(const RenderGeometry& geo) { m_cachedEdgeGeo = geo; }

    /// 获取缓存的边线渲染几何
    const RenderGeometry& cachedEdgeGeometry() const { return m_cachedEdgeGeo; }

    /// 是否有可渲染面数据
    bool hasRenderData() const { return m_cachedGeo.vertexCount > 0; }

    /// 是否有可渲染边线数据
    bool hasEdgeData() const { return m_cachedEdgeGeo.vertexCount > 0; }

    /// 材质索引（用于 RenderQueue 排序合并）
    uint16_t materialIndex() const { return m_materialIndex; }
    void setMaterialIndex(uint16_t idx) { m_materialIndex = idx; }

    // --- 面管理 ---

    void setFaces(std::vector<Face> faces) { m_faces = std::move(faces); }
    const std::vector<Face>& faces() const { return m_faces; }
    uint32_t faceCount() const { return static_cast<uint32_t>(m_faces.size()); }

    /// 按 pickId 查找面（线性搜索，面数不多时足够）
    Face* findFaceByPickId(uint32_t pickId);
    const Face* findFaceByPickId(uint32_t pickId) const;

    // --- 面选择 ---

    void selectFace(uint32_t faceIndex);
    void deselectFace(uint32_t faceIndex);
    void toggleFace(uint32_t faceIndex);
    void clearFaceSelection();

private:
    RenderGeometry   m_cachedGeo;
    RenderGeometry   m_cachedEdgeGeo;
    uint16_t         m_materialIndex = 0xFFFF;
    std::vector<Face> m_faces;
};

} // namespace MulanGeo::Engine
