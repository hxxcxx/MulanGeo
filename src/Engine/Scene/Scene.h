/**
 * @file Scene.h
 * @brief 场景管理器，管理场景节点树
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "SceneNode.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace MulanGeo::Engine {

class Scene {
public:
    Scene() {
        m_root = std::make_unique<SceneNode>(MulanGeo::BaseNodeType, "__root__");
    }

    // --- 根节点 ---

    SceneNode* root() { return m_root.get(); }
    const SceneNode* root() const { return m_root.get(); }

    // --- 添加节点 ---

    // 添加基类节点到根（装配体/分组用）
    SceneNode* addNode(std::string name, uint32_t pickId = 0) {
        return addNode(root(), std::move(name), pickId);
    }

    // 添加基类节点到指定父节点
    SceneNode* addNode(SceneNode* parent, std::string name, uint32_t pickId = 0) {
        auto child = std::make_unique<SceneNode>(MulanGeo::BaseNodeType, std::move(name), pickId);
        return parent->addChild(std::move(child));
    }

    // 添加预构造的节点到根（可以是任意派生类型）
    SceneNode* addChild(std::unique_ptr<SceneNode> child) {
        return addChild(root(), std::move(child));
    }

    // 添加预构造的节点到指定父节点
    SceneNode* addChild(SceneNode* parent, std::unique_ptr<SceneNode> child) {
        return parent->addChild(std::move(child));
    }

    // --- 查找 ---

    // 按 pickId 查找（深度优先）
    SceneNode* findByPickId(uint32_t pickId) {
        return findByPickId(root(), pickId);
    }

    // 按名称查找（深度优先，返回第一个匹配）
    SceneNode* findByName(std::string_view name) {
        return findByName(root(), name);
    }

    // --- 变换更新 ---

    // 更新整棵树的世界变换（从根开始级联）
    void updateWorldTransforms() {
        updateWorldTransform(root(), Mat4::identity());
    }

    // --- 遍历（回调式）---

    using NodeCallback = std::function<void(SceneNode&)>;

    // 遍历所有节点（深度优先）
    void traverse(const NodeCallback& callback) {
        traverseImpl(root(), callback);
    }

    // 只遍历可见节点（跳过不可见子树）
    void traverseVisible(const NodeCallback& callback) {
        traverseVisibleImpl(root(), callback);
    }

    // --- 统计 ---

    size_t nodeCount() const {
        size_t count = 0;
        countNodes(root(), count);
        return count;
    }

    // --- 清空 ---

    void clear() {
        m_root = std::make_unique<SceneNode>(MulanGeo::BaseNodeType, "__root__");
    }

private:
    // 深度优先查找
    SceneNode* findByPickId(SceneNode* node, uint32_t pickId) {
        if (!node) return nullptr;
        if (node->pickId() == pickId) return node;
        for (auto& child : node->children()) {
            auto* found = findByPickId(child.get(), pickId);
            if (found) return found;
        }
        return nullptr;
    }

    SceneNode* findByName(SceneNode* node, std::string_view name) {
        if (!node) return nullptr;
        if (node->name() == name) return node;
        for (auto& child : node->children()) {
            auto* found = findByName(child.get(), name);
            if (found) return found;
        }
        return nullptr;
    }

    // 级联更新世界变换
    void updateWorldTransform(SceneNode* node, const Mat4& parentWorld) {
        if (!node) return;
        auto newWorld = parentWorld * node->localTransform();
        node->m_worldTransform = newWorld;
        node->m_worldDirty = false;
        for (auto& child : node->children()) {
            updateWorldTransform(child.get(), newWorld);
        }
    }

    // 遍历所有节点
    void traverseImpl(SceneNode* node, const NodeCallback& callback) {
        if (!node) return;
        callback(*node);
        for (auto& child : node->children()) {
            traverseImpl(child.get(), callback);
        }
    }

    // 遍历可见节点
    void traverseVisibleImpl(SceneNode* node, const NodeCallback& callback) {
        if (!node || !node->isEffectivelyVisible()) return;
        callback(*node);
        for (auto& child : node->children()) {
            traverseVisibleImpl(child.get(), callback);
        }
    }

    void countNodes(const SceneNode* node, size_t& count) {
        if (!node) return;
        ++count;
        for (auto& child : node->children()) {
            countNodes(child.get(), count);
        }
    }

    std::unique_ptr<SceneNode> m_root;
};

} // namespace MulanGeo::Engine
