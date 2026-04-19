#include "Camera.h"
#include "../Math/AABB.h"

#include <algorithm>
#include <cmath>

namespace MulanGeo::Engine {

void Camera::orbit(double dx, double dy) {
    // 弧球旋转：将屏幕拖拽映射为 3D 旋转
    // 旋转轴垂直于拖拽方向，在相机平面上
    Vec3 right = computeRight();
    Vec3 up    = computeUp();

    // 屏幕水平拖拽 → 绕 up 轴旋转，屏幕垂直拖拽 → 绕 right 轴旋转
    // 组合旋转轴 = up * dx + right * dy（注意 dx 反向，因为向右拖应逆时针绕 up）
    Vec3 axis = right * static_cast<double>(dy) - up * static_cast<double>(dx);
    double len = axis.length();
    if (len < 1e-10) return;

    axis = axis / len;
    double angle = std::sqrt(static_cast<double>(dx * dx + dy * dy)) * m_orbitSpeed;

    Quat deltaQ = Quat::fromAxisAngle(axis, angle);
    m_rotation = (deltaQ * m_rotation).normalized();
}

void Camera::pan(double dx, double dy) {
    Vec3 right = computeRight();
    Vec3 up    = computeUp();
    double scale = (m_ortho ? m_orthoSize : m_distance) * m_panSpeed;
    m_target = m_target - right * (dx * scale);
    m_target = m_target + up    * (dy * scale);
}

void Camera::zoom(double delta) {
    if (m_ortho) {
        double factor = std::pow(m_zoomSpeed, delta);
        m_orthoSize *= factor;
        m_orthoSize = std::max(m_orthoSize, m_minDistance);
    } else {
        double factor = std::pow(m_zoomSpeed, delta);
        m_distance *= factor;
        m_distance = std::max(m_distance, m_minDistance);
    }
}

void Camera::fitToBox(const AABB& box, double padding) {
    m_target = box.center();
    double radius = (box.max - box.min).length() * 0.5;

    if (m_ortho) {
        m_orthoSize = radius * padding;
    } else {
        double halfFov = m_fovY * 0.5;
        m_distance = radius * padding / std::sin(halfFov);
    }

    if (m_distance < m_minDistance) m_distance = radius * 5.0 + 1.0;
}

Vec3 Camera::eyePosition() const {
    return m_target + computeOffset();
}

Mat4 Camera::viewMatrix() const {
    return Mat4::lookAt(eyePosition(), m_target, computeUp());
}

Mat4 Camera::projectionMatrix() const {
    if (m_ortho) {
        double h = m_orthoSize;
        double w = h * aspect();
        return Mat4::ortho(-w, w, -h, h, m_nearZ, m_farZ);
    }
    return Mat4::perspective(m_fovY, aspect(), m_nearZ, m_farZ);
}

Mat4 Camera::viewProjectionMatrix() const {
    return projectionMatrix() * viewMatrix();
}

Frustum Camera::frustum() const {
    return Frustum::fromViewProjection(viewProjectionMatrix());
}

// --- private ---

Vec3 Camera::computeOffset() const {
    // 四元数旋转 (0, 0, -1) 方向，再乘以距离
    // 初始相机朝 -Z 看（Z 轴朝上时），偏移量指向相机位置
    Mat4 rotMat = m_rotation.toMat4();
    Vec3 baseDir = {0, 0, -1}; // 相机默认朝 -Z 看
    Vec3 offset = rotMat.transformDir(baseDir);
    return offset * m_distance;
}

Vec3 Camera::computeRight() const {
    Mat4 rotMat = m_rotation.toMat4();
    return rotMat.transformDir(Vec3::unitX()).normalized();
}

Vec3 Camera::computeUp() const {
    Mat4 rotMat = m_rotation.toMat4();
    return rotMat.transformDir(Vec3::unitY()).normalized();
}

} // namespace MulanGeo::Engine
