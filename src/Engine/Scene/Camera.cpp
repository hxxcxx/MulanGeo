#include "Camera.h"
#include "../Math/AABB.h"

#include <algorithm>
#include <cmath>

namespace MulanGeo::Engine {

void Camera::setRotation(double theta, double phi) {
    m_theta = theta;
    m_phi = phi;
    clampPhi();
}

void Camera::orbit(double dx, double dy) {
    m_theta -= dx * m_orbitSpeed;
    m_phi   += dy * m_orbitSpeed;
    clampPhi();
}

void Camera::pan(double dx, double dy) {
    Vec3 right = computeRight();
    Vec3 up    = computeUp();
    double scale = m_distance * m_panSpeed;
    m_target = m_target - right * (dx * scale);
    m_target = m_target + up    * (dy * scale);
}

void Camera::zoom(double delta) {
    double factor = std::pow(m_zoomSpeed, delta);
    m_distance *= factor;
    m_distance = std::max(m_distance, m_minDistance);
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
    return Mat4::lookAt(eyePosition(), m_target, {0, 0, 1});
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
    double cosPhi = std::cos(m_phi);
    return {
        m_distance * cosPhi * std::cos(m_theta),
        m_distance * cosPhi * std::sin(m_theta),
        m_distance * std::sin(m_phi),
    };
}

Vec3 Camera::computeRight() const {
    return Vec3{std::cos(m_theta + detail::kPi / 2), std::sin(m_theta + detail::kPi / 2), 0}.normalized();
}

Vec3 Camera::computeUp() const {
    return Vec3::cross(computeRight(), (m_target - eyePosition()).normalized()).normalized();
}

void Camera::clampPhi() {
    const double maxPhi = detail::kPi / 2.0 - 0.01;
    if (m_phi > maxPhi)  m_phi = maxPhi;
    if (m_phi < -maxPhi) m_phi = -maxPhi;
}

} // namespace MulanGeo::Engine
