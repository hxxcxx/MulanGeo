/*
 * 网格节点 — 持有 IO 层的 MeshPart 引用
 *
 * MeshNode 属于 IO 模块，因为它直接关联 IO::MeshPart。
 * 继承自 Engine::SceneNode，通过 as<MeshNode>() 安全转换。
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
