/**
 * @file SceneNode.h
 * @brief 场景节点基类，层级、变换与可见性管理
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../Math/Mat4.h"
#include "../Math/AABB.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace MulanGeo {

// 节点类型标签 — 各模块可自行分配值
using NodeType = uint8_t;

// Engine 保留范围: 0
constexpr NodeType BaseNodeType = 0;

} // namespace MulanGeo

namespace MulanGeo::Engine {

// ============================================================
// 场景节点基类 — 只管层级、变换、可见性
// ============================================================

class Scene;  // 友元前向声明

class SceneNode {
    friend class Scene;
public:
    virtual ~SceneNode() = default;

    // --- 类型查询 ---

    MulanGeo::NodeType type() const { return m_type; }

    // 通用类型检查
    bool isType(MulanGeo::NodeType t) const { return m_type == t; }

    // 安全转换（失败返回 nullptr）
    template<typename T>
    T* as() { return (m_type == T::kType) ? static_cast<T*>(this) : nullptr; }

    template<typename T>
    const T* as() const { return (m_type == T::kType) ? static_cast<const T*>(this) : nullptr; }

    // --- 构造（子类调用）---

protected:
    explicit SceneNode(MulanGeo::NodeType type, std::string name = {}, uint32_t pickId = 0)
        : m_type(type), m_name(std::move(name)), m_pickId(pickId) {}

public:
    // 禁止拷贝
    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;

    // --- 层级 ---

    SceneNode* parent() const { return m_parent; }

    const std::vector<std::unique_ptr<SceneNode>>& children() const {
        return m_children;
    }

    // 添加子节点，返回原始指针
    SceneNode* addChild(std::unique_ptr<SceneNode> child) {
        child->m_parent = this;
        m_children.push_back(std::move(child));
        return m_children.back().get();
    }

    // 移除并返回子节点
    std::unique_ptr<SceneNode> removeChild(SceneNode* child);

    size_t childCount() const { return m_children.size(); }

    // --- 名称 ---

    std::string_view name() const { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }

    // --- 可见性 ---

    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    // 是否实际可见（自身 + 所有祖先都可见）
    bool isEffectivelyVisible() const {
        if (!m_visible) return false;
        return m_parent ? m_parent->isEffectivelyVisible() : true;
    }

    // --- 拾取 ---

    uint32_t pickId() const { return m_pickId; }
    void setPickId(uint32_t id) { m_pickId = id; }

    // --- 变换 ---

    const Mat4& localTransform() const { return m_localTransform; }
    void setLocalTransform(const Mat4& t) { m_localTransform = t; m_worldDirty = true; }

    const Mat4& worldTransform() const { return m_worldTransform; }

    // 标记世界变换需要重新计算
    void markDirty() { m_worldDirty = true; }
    bool isWorldDirty() const { return m_worldDirty; }

    // --- 包围盒（世界空间）---

    const AABB& boundingBox() const { return m_bounds; }
    void setBoundingBox(const AABB& bounds) { m_bounds = bounds; }

    // --- 选择状态 ---

    bool selected() const { return m_selected; }
    void setSelected(bool s) { m_selected = s; }

private:
    MulanGeo::NodeType m_type;
    std::string m_name;
    uint32_t    m_pickId   = 0;
    bool        m_visible  = true;
    bool        m_selected = false;

    Mat4     m_localTransform = Mat4::identity();
    Mat4     m_worldTransform = Mat4::identity();
    bool     m_worldDirty     = true;
    AABB     m_bounds;

    SceneNode*  m_parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> m_children;
};

} // namespace MulanGeo::Engine
