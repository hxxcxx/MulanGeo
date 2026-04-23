#include "SceneNode.h"

namespace MulanGeo::Engine {

SceneNode* SceneNode::addChild(std::unique_ptr<SceneNode> child) {
    child->m_parent = this;
    child->markDirty(DirtyFlag::Transform);
    m_children.push_back(std::move(child));
    return m_children.back().get();
}

std::unique_ptr<SceneNode> SceneNode::removeChild(SceneNode* child) {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        if (it->get() == child) {
            child->m_parent = nullptr;
            auto ptr = std::move(*it);
            m_children.erase(it);
            return ptr;
        }
    }
    return nullptr;
}

bool SceneNode::isEffectivelyVisible() const {
    if (!m_visible) return false;
    return m_parent ? m_parent->isEffectivelyVisible() : true;
}

void SceneNode::setVisible(bool v) {
    if (m_visible != v) {
        m_visible = v;
        markDirty(DirtyFlag::Visibility);
        // 级联标记子节点
        for (auto& child : m_children)
            child->markDirty(DirtyFlag::Visibility);
    }
}

void SceneNode::setLocalTransform(const Mat4& t) {
    m_localTransform = t;
    markDirty(DirtyFlag::Transform);
    // 级联标记子节点需要重新计算世界变换
    for (auto& child : m_children)
        child->markDirty(DirtyFlag::Transform | DirtyFlag::Bounds);
}

} // namespace MulanGeo::Engine
