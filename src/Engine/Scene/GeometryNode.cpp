/**
 * @file GeometryNode.cpp
 * @brief GeometryNode 实现
 * @author hxxcxx
 * @date 2026-04-21
 */

#include "GeometryNode.h"

namespace MulanGeo::Engine {

GeometryNode::Face* GeometryNode::findFaceByPickId(uint32_t pickId) {
    for (auto& f : m_faces) {
        if (f.pickId == pickId) return &f;
    }
    return nullptr;
}

const GeometryNode::Face* GeometryNode::findFaceByPickId(uint32_t pickId) const {
    for (const auto& f : m_faces) {
        if (f.pickId == pickId) return &f;
    }
    return nullptr;
}

void GeometryNode::selectFace(uint32_t faceIndex) {
    if (faceIndex < m_faces.size()) {
        m_faces[faceIndex].selected = true;
        setSelected(true);
    }
}

void GeometryNode::deselectFace(uint32_t faceIndex) {
    if (faceIndex < m_faces.size()) {
        m_faces[faceIndex].selected = false;
    }
}

void GeometryNode::toggleFace(uint32_t faceIndex) {
    if (faceIndex < m_faces.size()) {
        m_faces[faceIndex].selected = !m_faces[faceIndex].selected;
    }
}

void GeometryNode::clearFaceSelection() {
    for (auto& f : m_faces) {
        f.selected = false;
    }
    setSelected(false);
}

} // namespace MulanGeo::Engine
