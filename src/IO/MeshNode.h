/**
 * @file MeshNode.h
 * @brief 网格节点，持有IO层MeshPart引用的场景节点
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "MulanGeo/Engine/Scene/SceneNode.h"
#include "MeshData.h"

namespace MulanGeo::IO {

// IO 模块的节点类型标签（Engine 预留 0，IO 从 1 开始）
constexpr MulanGeo::NodeType MeshNodeType = 1;

// ============================================================
// 网格节点 — 零件级别的场景节点
// ============================================================

class MeshNode final : public MulanGeo::Engine::SceneNode {
public:
    static constexpr MulanGeo::NodeType kType = MeshNodeType;

    using MeshPart = MulanGeo::IO::MeshPart;

    explicit MeshNode(std::string name = {}, uint32_t pickId = 0)
        : SceneNode(kType, std::move(name), pickId) {}

    // 网格数据（指向 IO 层，不拥有）
    const MeshPart* meshData() const { return m_meshData; }
    void setMeshData(const MeshPart* mesh) { m_meshData = mesh; }

private:
    const MeshPart* m_meshData = nullptr;
};

} // namespace MulanGeo::IO
