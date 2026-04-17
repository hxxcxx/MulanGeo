#include "SceneNode.h"

namespace MulanGeo::Engine {

SceneNode* SceneNode::addChild(std::unique_ptr<SceneNode> child) {
    child->m_parent = this;
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

} // namespace MulanGeo::Engine
