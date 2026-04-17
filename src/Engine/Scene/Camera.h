/**
 * @file Camera.h
 * @brief 轨道相机，支持旋转、平移、缩放交互
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "../Math/Vec3.h"
#include "../Math/Mat4.h"
#include "Frustum.h"

namespace MulanGeo::Engine {

struct AABB;

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

    void setTarget(const Vec3& target) { m_target = target; }
    const Vec3& target() const { return m_target; }

    void setDistance(double dist) { m_distance = dist; }
    double distance() const { return m_distance; }

    void setRotation(double theta, double phi);

    // --- 交互 ---

    void orbit(double dx, double dy);
    void pan(double dx, double dy);
    void zoom(double delta);

    // 适配包围盒（自动调整距离让整个 AABB 可见）
    void fitToBox(const AABB& box, double padding = 1.2);

    // --- 矩阵计算 ---

    Vec3 eyePosition() const;
    Mat4 viewMatrix() const;
    Mat4 projectionMatrix() const;
    Mat4 viewProjectionMatrix() const;
    Frustum frustum() const;

private:
    Vec3 computeOffset() const;
    Vec3 computeRight() const;
    Vec3 computeUp() const;
    void clampPhi();

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
