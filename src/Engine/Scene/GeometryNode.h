/**
 * @file GeometryNode.h
 * @brief 几何场景节点，持有按面拆分的网格数据，支持面级别选择
 * @author hxxcxx
 * @date 2026-04-21
 */

#pragma once

#include "SceneNode.h"
#include "../Geometry/MeshGeometry.h"

#include <cstdint>
#include <vector>

namespace MulanGeo::Engine {

// ============================================================
// 几何节点 — 场景中可渲染的几何体
//
// 数据按面拆分存储，每个面有独立的 pickId，
// 支持面级别的拾取、选择和高亮。
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
    std::vector<Face> m_faces;
};

} // namespace MulanGeo::Engine
