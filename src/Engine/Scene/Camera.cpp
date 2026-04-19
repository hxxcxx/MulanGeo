#include "Camera.h"
#include "../Math/AABB.h"

#include <algorithm>
#include <cmath>

namespace MulanGeo::Engine {

// ============================================================
// 模式切换
// ============================================================

void Camera::setMode(CameraMode mode) {
    if (m_mode == mode) return;

    if (m_mode == CameraMode::Turntable && mode == CameraMode::Trackball) {
        // Turntable → Trackball：从 yaw/pitch 构建等效四元数
        // 先绕 Z 旋转 yaw，再绕本地 X 旋转 -(pi/2 - pitch)
        // 等效于：相机在 yaw/pitch 所描述的位置看向 target
        Quat qYaw   = Quat::fromAxisAngle(Vec3::unitZ(), m_yaw);
        Quat qPitch = Quat::fromAxisAngle(Vec3::unitX(), m_pitch - detail::kPi * 0.5);
        m_rotation = (qYaw * qPitch).normalized();
    } else if (m_mode == CameraMode::Trackball && mode == CameraMode::Turntable) {
        // Trackball → Turntable：从四元数中提取 yaw/pitch
        Vec3 fwd = trackballForward();
        m_pitch = std::asin(std::clamp(fwd.z, -1.0, 1.0));
        m_yaw   = std::atan2(fwd.y, fwd.x);
        m_pitch = std::clamp(m_pitch, kMinPitch, kMaxPitch);
    }

    m_mode = mode;
}

// ============================================================
// 交互
// ============================================================

void Camera::setYawPitch(double yaw, double pitch) {
    m_yaw = yaw;
    m_pitch = std::clamp(pitch, kMinPitch, kMaxPitch);
}

void Camera::orbit(double dx, double dy) {
    if (m_mode == CameraMode::Turntable) {
        // 水平 → yaw（绕世界 Z），垂直 → pitch（仰角）
        m_yaw   -= dx * m_orbitSpeed;
        m_pitch += dy * m_orbitSpeed;
        m_pitch  = std::clamp(m_pitch, kMinPitch, kMaxPitch);
    } else {
        // Trackball: 屏幕拖拽映射为绕视图空间轴的旋转
        // 旋转角度正比于像素位移
        double angle = std::sqrt(dx * dx + dy * dy) * m_orbitSpeed;
        if (angle < 1e-10) return;

        // 计算旋转轴：在世界空间中，垂直于拖拽方向
        // 水平拖拽(dx) → 绕 camera up 旋转
        // 垂直拖拽(dy) → 绕 camera right 旋转
        Vec3 camRight = trackballRight();
        Vec3 camUp    = trackballUp();

        // 旋转轴 = -(up * dx - right * dy)，取反使拖拽方向与旋转方向一致
        Vec3 axis = camRight * dy - camUp * dx;
        double len = axis.length();
        if (len < 1e-10) return;
        axis = axis / len;

        // 在世界坐标系中施加旋转：左乘 deltaQ
        Quat deltaQ = Quat::fromAxisAngle(axis, angle);
        m_rotation = (deltaQ * m_rotation).normalized();
    }
}

void Camera::pan(double dx, double dy) {
    Vec3 r = right();
    Vec3 u = up();
    double scale = (m_ortho ? m_orthoSize : m_distance) * m_panSpeed;
    m_target = m_target - r * (dx * scale) - u * (dy * scale);
}

void Camera::zoom(double delta) {
    if (m_ortho) {
        double factor = std::pow(m_zoomSpeed, delta);
        m_orthoSize = std::max(m_orthoSize * factor, m_minDistance);
    } else {
        double factor = std::pow(m_zoomSpeed, delta);
        m_distance = std::max(m_distance * factor, m_minDistance);
    }
}

void Camera::fitToBox(const AABB& box, double padding) {
    m_target = box.center();
    double radius = (box.max - box.min).length() * 0.5;

    if (m_ortho) {
        m_orthoSize = radius * padding;
        m_distance  = radius * padding * 2.0;
    } else {
        double halfFov = m_fovY * 0.5;
        m_distance = radius * padding / std::sin(halfFov);
    }

    if (m_distance < m_minDistance) m_distance = radius * 5.0 + 1.0;

    // 动态调整近远裁面包住整个场景
    m_nearZ = m_distance * 0.01;
    m_farZ  = m_distance * 10.0;
}

// ============================================================
// 矩阵计算
// ============================================================

Vec3 Camera::eyePosition() const {
    Vec3 fwd = forward();
    return m_target - fwd * m_distance;
}

Mat4 Camera::viewMatrix() const {
    if (m_mode == CameraMode::Turntable) {
        // Turntable: 用 lookAt，worldUp = Z
        // pitch 被钳位在 ±89°，不会退化
        Vec3 eye = eyePosition();
        return Mat4::lookAt(eye, m_target, Vec3::unitZ());
    } else {
        // Trackball: 从四元数直接构建 view matrix
        // 四元数 m_rotation 描述"从初始朝向到当前朝向"的旋转
        // 相机坐标系：right=X, up=Y, forward=-Z（OpenGL/Vulkan 惯例）
        Vec3 r   = trackballRight();
        Vec3 u   = trackballUp();
        Vec3 fwd = trackballForward();
        Vec3 eye = m_target - fwd * m_distance;

        // View matrix = [R|t]，旋转部分转置后嵌入平移
        Mat4 v;
        v.m[0][0] = r.x;    v.m[1][0] = r.y;    v.m[2][0] = r.z;    v.m[3][0] = -Vec3::dot(r, eye);
        v.m[0][1] = u.x;    v.m[1][1] = u.y;    v.m[2][1] = u.z;    v.m[3][1] = -Vec3::dot(u, eye);
        v.m[0][2] = -fwd.x; v.m[1][2] = -fwd.y; v.m[2][2] = -fwd.z; v.m[3][2] = Vec3::dot(fwd, eye);
        v.m[0][3] = 0;      v.m[1][3] = 0;      v.m[2][3] = 0;      v.m[3][3] = 1;
        return v;
    }
}

Mat4 Camera::rotationMatrix() const {
    Vec3 r   = right();
    Vec3 u   = up();
    Vec3 fwd = forward();

    Mat4 rm;
    rm.m[0][0] = r.x;    rm.m[1][0] = r.y;    rm.m[2][0] = r.z;    rm.m[3][0] = 0;
    rm.m[0][1] = u.x;    rm.m[1][1] = u.y;    rm.m[2][1] = u.z;    rm.m[3][1] = 0;
    rm.m[0][2] = -fwd.x; rm.m[1][2] = -fwd.y; rm.m[2][2] = -fwd.z; rm.m[3][2] = 0;
    rm.m[0][3] = 0;      rm.m[1][3] = 0;      rm.m[2][3] = 0;      rm.m[3][3] = 1;
    return rm;
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

// ============================================================
// Turntable 方向计算（Z-up 球面坐标）
// ============================================================

Vec3 Camera::turntableForward() const {
    double cp = std::cos(m_pitch);
    return Vec3{cp * std::cos(m_yaw), cp * std::sin(m_yaw), std::sin(m_pitch)};
}

Vec3 Camera::turntableRight() const {
    Vec3 fwd = turntableForward();
    return Vec3::cross(fwd, Vec3::unitZ()).normalized();
}

Vec3 Camera::turntableUp() const {
    Vec3 fwd = turntableForward();
    Vec3 r   = Vec3::cross(fwd, Vec3::unitZ()).normalized();
    return Vec3::cross(r, fwd).normalized();
}

// ============================================================
// Trackball 方向计算（四元数）
// ============================================================

Vec3 Camera::trackballForward() const {
    // 四元数旋转参考方向 (0,1,0) 得到 forward
    // 初始相机朝 +Y 看（Z-up 惯例：初始 forward = +Y）
    // q * v * q^-1 展开
    Mat4 rm = m_rotation.toMat4();
    return rm.transformDir(Vec3::unitY()).normalized();
}

Vec3 Camera::trackballRight() const {
    Mat4 rm = m_rotation.toMat4();
    return rm.transformDir(Vec3::unitX()).normalized();
}

Vec3 Camera::trackballUp() const {
    Mat4 rm = m_rotation.toMat4();
    return rm.transformDir(Vec3::unitZ()).normalized();
}

// ============================================================
// 通用方向分发
// ============================================================

Vec3 Camera::forward() const {
    return (m_mode == CameraMode::Turntable) ? turntableForward() : trackballForward();
}

Vec3 Camera::right() const {
    return (m_mode == CameraMode::Turntable) ? turntableRight() : trackballRight();
}

Vec3 Camera::up() const {
    return (m_mode == CameraMode::Turntable) ? turntableUp() : trackballUp();
}

} // namespace MulanGeo::Engine
