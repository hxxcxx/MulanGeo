/**
 * @file Camera.h
 * @brief 双模式轨道相机 — Turntable + Trackball
 *
 * 支持两种旋转模式：
 *   1. Turntable：yaw/pitch 角度驱动，Z-up 约束，适合机械类查看（有上方向约束）
 *   2. Trackball：四元数驱动，自由旋转无死角，适合任意方向查看（默认）
 *
 * 两种模式共享：target、distance、投影参数、pan、zoom 逻辑。
 * 切换模式时自动同步状态（Turntable→Trackball / Trackball→Turntable）。
 *
 * Z-up 坐标系（CAD 惯例）。
 *
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include <cmath>

#include "../Math/Vec3.h"
#include "../Math/Quat.h"
#include "../Math/Mat4.h"
#include "Frustum.h"

namespace MulanGeo::Engine {

namespace detail {
constexpr double kPi = 3.14159265358979323846;
}

struct AABB;

/// 相机旋转模式
enum class CameraMode : uint8_t {
    Turntable,   ///< yaw/pitch 转台，世界 Z-up 约束
    Trackball,   ///< 四元数自由旋转（arcball）
};

class Camera {
public:
    Camera() = default;

    // ==================== 模式控制 ====================

    CameraMode mode() const { return m_mode; }
    void setMode(CameraMode mode);

    // ==================== 视口 ====================

    void setViewport(int width, int height) {
        m_width = width;
        m_height = height;
    }

    int width()  const { return m_width; }
    int height() const { return m_height; }
    double aspect() const { return m_height > 0 ? double(m_width) / m_height : 1.0; }

    // ==================== 投影参数 ====================

    void setFieldOfView(double fovY) { m_fovY = fovY; }
    void setClipPlanes(double nearZ, double farZ) { m_nearZ = nearZ; m_farZ = farZ; }
    void setOrthographic(bool ortho) { m_ortho = ortho; }

    double fieldOfView() const { return m_fovY; }
    bool   isOrthographic() const { return m_ortho; }
    double nearPlane() const { return m_nearZ; }
    double farPlane()  const { return m_farZ; }

    // ==================== 轨道参数 ====================

    void setTarget(const Vec3& target) { m_target = target; }
    const Vec3& target() const { return m_target; }

    void setDistance(double dist) { m_distance = dist; }
    double distance() const { return m_distance; }

    double orthoSize() const { return m_orthoSize; }
    void setOrthoSize(double s) { m_orthoSize = s; }

    // Turntable 专用
    double yaw()   const { return m_yaw; }
    double pitch() const { return m_pitch; }
    void setYawPitch(double yaw, double pitch);

    // Trackball 专用
    const Quat& rotation() const { return m_rotation; }
    void setRotation(const Quat& q) { m_rotation = q.normalized(); }

    // ==================== 交互 ====================

    void orbit(double dx, double dy);
    void pan(double dx, double dy);
    void zoom(double delta);

    void fitToBox(const AABB& box, double padding = 1.2);

    // ==================== 速度参数 ====================

    void setOrbitSpeed(double s)  { m_orbitSpeed = s; }
    void setPanSpeed(double s)    { m_panSpeed = s; }
    void setZoomSpeed(double s)   { m_zoomSpeed = s; }

    double orbitSpeed() const { return m_orbitSpeed; }
    double panSpeed()   const { return m_panSpeed; }
    double zoomSpeed()  const { return m_zoomSpeed; }

    // ==================== 矩阵计算 ====================

    Vec3 eyePosition() const;
    Mat4 viewMatrix() const;
    Mat4 projectionMatrix() const;
    Mat4 viewProjectionMatrix() const;
    Mat4 rotationMatrix() const;
    Frustum frustum() const;

private:
    // --- Turntable 内部 ---
    Vec3 turntableForward() const;
    Vec3 turntableRight() const;
    Vec3 turntableUp() const;

    // --- Trackball 内部 ---
    Vec3 trackballForward() const;
    Vec3 trackballRight() const;
    Vec3 trackballUp() const;

    // --- 通用方向（根据模式分发）---
    Vec3 forward() const;
    Vec3 right() const;
    Vec3 up() const;

    // === 状态 ===

    CameraMode m_mode = CameraMode::Trackball;  // 默认 Trackball

    // 轨道共享参数
    Vec3   m_target   = {0, 0, 0};
    double m_distance = 10.0;

    // Turntable 参数（Z-up 球面坐标）
    double m_yaw   = detail::kPi * 0.25;   // 方位角 45°
    double m_pitch = detail::kPi * 0.33;   // 仰角 ~60°

    // Trackball 参数（四元数描述相机朝向）
    // 初始值在构造时通过 initTrackballRotation() 与 Turntable 同步
    Quat m_rotation = initTrackballRotation();

    static Quat initTrackballRotation() {
        constexpr double pi = 3.14159265358979323846;
        // q = Rot(Z, yaw - pi/2) * Rot(X, pitch)
        // yaw=pi*0.25, pitch=pi*0.33
        Quat qYaw   = Quat::fromAxisAngle(Vec3{0,0,1}, pi * 0.25 - pi * 0.5);
        Quat qPitch = Quat::fromAxisAngle(Vec3{1,0,0}, pi * 0.33);
        return (qYaw * qPitch).normalized();
    }

    // 投影参数
    int    m_width    = 800;
    int    m_height   = 600;
    double m_fovY     = detail::kPi / 4.0;    // 45°
    double m_nearZ    = 0.1;
    double m_farZ     = 1000.0;
    bool   m_ortho    = true;                  // 默认正交
    double m_orthoSize = 5.0;

    // 交互速度
    double m_orbitSpeed  = 0.005;   // 弧度/像素
    double m_panSpeed    = 0.003;
    double m_zoomSpeed   = 1.08;    // 每档 8% 缩放
    double m_minDistance = 0.001;

    // Turntable pitch 钳位
    static constexpr double kPi_      = 3.14159265358979323846;
    static constexpr double kMaxPitch =  kPi_ * 0.5 - 0.01;
    static constexpr double kMinPitch = -kPi_ * 0.5 + 0.01;
};

} // namespace MulanGeo::Engine
