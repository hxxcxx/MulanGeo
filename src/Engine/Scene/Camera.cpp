#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace MulanGeo::Engine {

// ============================================================
// 模式切换
// ============================================================

void Camera::setMode(CameraMode mode) {
    if (m_mode == mode) return;

    if (m_mode == CameraMode::Turntable && mode == CameraMode::Trackball) {
        Quat qYaw   = glm::angleAxis(m_yaw - detail::kPi * 0.5, Vec3{0, 0, 1});
        Quat qPitch = glm::angleAxis(m_pitch, Vec3{1, 0, 0});
        m_rotation = glm::normalize(qYaw * qPitch);
    } else if (m_mode == CameraMode::Trackball && mode == CameraMode::Turntable) {
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
        m_yaw   -= dx * m_orbitSpeed;
        m_pitch += dy * m_orbitSpeed;
        m_pitch  = std::clamp(m_pitch, kMinPitch, kMaxPitch);
    } else {
        double angle = std::sqrt(dx * dx + dy * dy) * m_orbitSpeed;
        if (angle < 1e-10) return;

        Vec3 camRight = trackballRight();
        Vec3 camUp    = trackballUp();

        Vec3 axis = camRight * dy - camUp * dx;
        double len = glm::length(axis);
        if (len < 1e-10) return;
        axis = axis / len;

        Quat deltaQ = glm::angleAxis(angle, axis);
        m_rotation = glm::normalize(deltaQ * m_rotation);
    }
}

void Camera::pan(double dx, double dy) {
    Vec3 r = right();
    Vec3 u = up();
    double scale = (m_ortho ? m_orthoSize : m_distance) * m_panSpeed;
    m_target = m_target - r * (dx * scale) + u * (dy * scale);
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
    double radius = glm::length(box.max - box.min) * 0.5;

    if (m_ortho) {
        m_orthoSize = radius * padding;
        m_distance  = radius * padding * 2.0;
    } else {
        double halfFov = m_fovY * 0.5;
        m_distance = radius * padding / std::sin(halfFov);
    }

    if (m_distance < m_minDistance) m_distance = radius * 5.0 + 1.0;

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
        Vec3 eye = eyePosition();
        return glm::lookAt(eye, m_target, Vec3{0, 0, 1});
    } else {
        Vec3 r   = trackballRight();
        Vec3 u   = trackballUp();
        Vec3 fwd = trackballForward();
        Vec3 eye = m_target - fwd * m_distance;

        Mat4 v(1.0);
        v[0][0] = r.x;    v[1][0] = r.y;    v[2][0] = r.z;    v[3][0] = -glm::dot(r, eye);
        v[0][1] = u.x;    v[1][1] = u.y;    v[2][1] = u.z;    v[3][1] = -glm::dot(u, eye);
        v[0][2] = -fwd.x; v[1][2] = -fwd.y; v[2][2] = -fwd.z; v[3][2] = glm::dot(fwd, eye);
        v[0][3] = 0;      v[1][3] = 0;      v[2][3] = 0;      v[3][3] = 1;
        return v;
    }
}

Mat4 Camera::rotationMatrix() const {
    Vec3 r   = right();
    Vec3 u   = up();
    Vec3 fwd = forward();

    Mat4 rm(1.0);
    rm[0][0] = r.x;    rm[1][0] = r.y;    rm[2][0] = r.z;    rm[3][0] = 0;
    rm[0][1] = u.x;    rm[1][1] = u.y;    rm[2][1] = u.z;    rm[3][1] = 0;
    rm[0][2] = -fwd.x; rm[1][2] = -fwd.y; rm[2][2] = -fwd.z; rm[3][2] = 0;
    rm[0][3] = 0;      rm[1][3] = 0;      rm[2][3] = 0;      rm[3][3] = 1;
    return rm;
}

Mat4 Camera::projectionMatrix() const {
    if (m_ortho) {
        double h = m_orthoSize;
        double w = h * aspect();
        return glm::ortho(-w, w, -h, h, m_nearZ, m_farZ);
    }
    return glm::perspective(m_fovY, aspect(), m_nearZ, m_farZ);
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
    return glm::normalize(glm::cross(fwd, Vec3{0, 0, 1}));
}

Vec3 Camera::turntableUp() const {
    Vec3 fwd = turntableForward();
    Vec3 r   = glm::normalize(glm::cross(fwd, Vec3{0, 0, 1}));
    return glm::normalize(glm::cross(r, fwd));
}

// ============================================================
// Trackball 方向计算（四元数）
// ============================================================

Vec3 Camera::trackballForward() const {
    Mat4 rm = glm::mat4_cast(m_rotation);
    return glm::normalize(Vec3(rm * Vec4{0, 1, 0, 0}));
}

Vec3 Camera::trackballRight() const {
    Mat4 rm = glm::mat4_cast(m_rotation);
    return glm::normalize(Vec3(rm * Vec4{1, 0, 0, 0}));
}

Vec3 Camera::trackballUp() const {
    Mat4 rm = glm::mat4_cast(m_rotation);
    return glm::normalize(Vec3(rm * Vec4{0, 0, 1, 0}));
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
