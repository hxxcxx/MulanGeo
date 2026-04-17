#include "Scene.h"

namespace MulanGeo::Engine {

Scene::Scene() {
    m_root = SceneNode::create(MulanGeo::BaseNodeType, "__root__");
}

SceneNode* Scene::addNode(std::string name, uint32_t pickId) {
    return addNode(root(), std::move(name), pickId);
}

SceneNode* Scene::addNode(SceneNode* parent, std::string name, uint32_t pickId) {
    auto child = SceneNode::create(MulanGeo::BaseNodeType, std::move(name), pickId);
    return parent->addChild(std::move(child));
}

SceneNode* Scene::addChild(std::unique_ptr<SceneNode> child) {
    return addChild(root(), std::move(child));
}

SceneNode* Scene::addChild(SceneNode* parent, std::unique_ptr<SceneNode> child) {
    return parent->addChild(std::move(child));
}

SceneNode* Scene::findByPickId(uint32_t pickId) {
    return findByPickId(root(), pickId);
}

SceneNode* Scene::findByName(std::string_view name) {
    return findByName(root(), name);
}

void Scene::updateWorldTransforms() {
    updateWorldTransform(root(), Mat4::identity());
}

void Scene::traverse(const NodeCallback& callback) {
    traverseImpl(root(), callback);
}

void Scene::traverseVisible(const NodeCallback& callback) {
    traverseVisibleImpl(root(), callback);
}

size_t Scene::nodeCount() const {
    size_t count = 0;
    countNodes(root(), count);
    return count;
}

void Scene::clear() {
    m_root = SceneNode::create(MulanGeo::BaseNodeType, "__root__");
}

// --- private ---

SceneNode* Scene::findByPickId(SceneNode* node, uint32_t pickId) {
    if (!node) return nullptr;
    if (node->pickId() == pickId) return node;
    for (auto& child : node->children()) {
        auto* found = findByPickId(child.get(), pickId);
        if (found) return found;
    }
    return nullptr;
}

SceneNode* Scene::findByName(SceneNode* node, std::string_view name) {
    if (!node) return nullptr;
    if (node->name() == name) return node;
    for (auto& child : node->children()) {
        auto* found = findByName(child.get(), name);
        if (found) return found;
    }
    return nullptr;
}

void Scene::updateWorldTransform(SceneNode* node, const Mat4& parentWorld) {
    if (!node) return;
    auto newWorld = parentWorld * node->localTransform();
    node->m_worldTransform = newWorld;
    node->m_worldDirty = false;
    for (auto& child : node->children()) {
        updateWorldTransform(child.get(), newWorld);
    }
}

void Scene::traverseImpl(SceneNode* node, const NodeCallback& callback) {
    if (!node) return;
    callback(*node);
    for (auto& child : node->children()) {
        traverseImpl(child.get(), callback);
    }
}

void Scene::traverseVisibleImpl(SceneNode* node, const NodeCallback& callback) {
    if (!node || !node->isEffectivelyVisible()) return;
    callback(*node);
    for (auto& child : node->children()) {
        traverseVisibleImpl(child.get(), callback);
    }
}

void Scene::countNodes(const SceneNode* node, size_t& count) const {
    if (!node) return;
    ++count;
    for (auto& child : node->children()) {
        countNodes(child.get(), count);
    }
}

} // namespace MulanGeo::Engine
