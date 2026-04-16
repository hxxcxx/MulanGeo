/**
 * @file Camera.h
 * @brief 轨道相机，支持旋转、平移、缩放交互
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../Math/Vec3.h"
#include "../Math/Mat4.h"
#include "Frustum.h"

#include <cmath>

namespace MulanGeo::Engine {

class Camera {
public:
    Camera() = default;

    // --- 视口 ---

    void setViewport(int width, int height) {
        m_width = width;
        m_height = height;
    }

    int width()  const { return m_width; }
    int height() const { return m_height; }
    double aspect() const { return m_height > 0 ? double(m_width) / m_height : 1.0; }

    // --- 投影参数 ---

    void setFieldOfView(double fovY) { m_fovY = fovY; }
    void setClipPlanes(double nearZ, double farZ) { m_nearZ = nearZ; m_farZ = farZ; }
    void setOrthographic(bool ortho) { m_ortho = ortho; }

    double fieldOfView() const { return m_fovY; }
    bool   isOrthographic() const { return m_ortho; }

    // --- 轨道参数 ---

    // 目标点（相机围绕旋转的中心）
    void setTarget(const Vec3& target) { m_target = target; }
    const Vec3& target() const { return m_target; }

    // 距离（相机到目标的距离）
    void setDistance(double dist) { m_distance = dist; }
    double distance() const { return m_distance; }

    // 旋转角（球坐标系：theta=方位角, phi=仰角）
    void setRotation(double theta, double phi) {
        m_theta = theta;
        m_phi = phi;
        clampPhi();
    }

    // --- 交互 ---

    // 轨道旋转（dx/dy 为像素偏移量）
    void orbit(double dx, double dy) {
        m_theta -= dx * m_orbitSpeed;
        m_phi   += dy * m_orbitSpeed;
        clampPhi();
    }

    // 平移（在视图平面上移动目标点）
    void pan(double dx, double dy) {
        Vec3 right = computeRight();
        Vec3 up    = computeUp();
        double scale = m_distance * m_panSpeed;
        m_target = m_target - right * dx * scale;
        m_target = m_target + up * dy * scale;
    }

    // 缩放（调整距离）
    void zoom(double delta) {
        double factor = std::pow(m_zoomSpeed, delta);
        m_distance *= factor;
        m_distance = std::max(m_distance, m_minDistance);
    }

    // 适配包围盒（自动调整距离让整个 AABB 可见）
    void fitToBox(const AABB& box, double padding = 1.2) {
        m_target = box.center();
        double radius = (box.max - box.min).length() * 0.5;

        if (m_ortho) {
            m_orthoSize = radius * padding;
        } else {
            // 根据视角算距离，让球体刚好充满视口
            double halfFov = m_fovY * 0.5;
            m_distance = radius * padding / std::sin(halfFov);
        }

        // 如果包围盒退化，给一个合理的默认距离
        if (m_distance < m_minDistance) m_distance = radius * 5.0 + 1.0;
    }

    // --- 矩阵计算 ---

    // 相机在世界空间中的位置
    Vec3 eyePosition() const {
        return m_target + computeOffset();
    }

    Mat4 viewMatrix() const {
        return Mat4::lookAt(eyePosition(), m_target, {0, 0, 1});
    }

    Mat4 projectionMatrix() const {
        if (m_ortho) {
            double h = m_orthoSize;
            double w = h * aspect();
            return Mat4::ortho(-w, w, -h, h, m_nearZ, m_farZ);
        }
        return Mat4::perspective(m_fovY, aspect(), m_nearZ, m_farZ);
    }

    // 组合矩阵
    Mat4 viewProjectionMatrix() const {
        return projectionMatrix() * viewMatrix();
    }

    // 视锥体（用于场景裁剪）
    Frustum frustum() const {
        return Frustum::fromViewProjection(viewProjectionMatrix());
    }

private:
    // 球坐标 → 直角坐标偏移
    Vec3 computeOffset() const {
        double cosPhi = std::cos(m_phi);
        return {
            m_distance * cosPhi * std::cos(m_theta),
            m_distance * cosPhi * std::sin(m_theta),
            m_distance * std::sin(m_phi),
        };
    }

    Vec3 computeRight() const {
        return Vec3{std::cos(m_theta + M_PI_2), std::sin(m_theta + M_PI_2), 0}.normalized();
    }

    Vec3 computeUp() const {
        return Vec3::cross(computeRight(), (m_target - eyePosition()).normalized()).normalized();
    }

    void clampPhi() {
        // 限制仰角避免万向锁，留一点余量
        const double maxPhi = M_PI_2 - 0.01;
        if (m_phi > maxPhi)  m_phi = maxPhi;
        if (m_phi < -maxPhi) m_phi = -maxPhi;
    }

    // 轨道参数
    Vec3   m_target     = {0, 0, 0};
    double m_distance   = 10.0;
    double m_theta      = M_PI * 0.25;  // 方位角（弧度）
    double m_phi        = M_PI * 0.15;  // 仰角（弧度）

    // 投影参数
    int    m_width      = 800;
    int    m_height     = 600;
    double m_fovY       = M_PI / 4.0;   // 45°
    double m_nearZ      = 0.01;
    double m_farZ       = 10000.0;
    bool   m_ortho      = false;
    double m_orthoSize  = 5.0;

    // 交互速度
    double m_orbitSpeed   = 0.005;
    double m_panSpeed     = 0.002;
    double m_zoomSpeed    = 1.1;
    double m_minDistance  = 0.001;
};

} // namespace MulanGeo::Engine
