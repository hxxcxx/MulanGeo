/**
 * @file Camera.h
 * @brief 相机策略模式托管类 — 持有 RotationMode 实现，按模式创建
 *
 * 职责：
 *  - 投影参数（fov、near/far、ortho）
 *  - 轨道参数（target、distance）
 *  - pan / zoom / fitToBox 等与旋转无关的操作
 *  - 旋转操作全部委托给当前激活的 RotationMode
 *
 * 两种模式的旋转状态完全独立，切换时不做有损转换。
 * Z-up 坐标系（CAD 惯例）。
 *
 * @author hxxcxx
 * @date 2026-04-25
 */

#pragma once

#include <cstdint>
#include <memory>

#include "../../Math/Math.h"
#include "../../Math/AABB.h"
#include "../Frustum.h"
#include "RotationMode.h"

namespace MulanGeo::Engine {

/// 相机旋转模式
enum class CameraMode : uint8_t {
    Turntable,   ///< yaw/pitch 转台，世界 Z-up 约束
    Trackball,   ///< 四元数自由旋转（arcball）
};

class Camera {
public:
    /// @param initialMode 初始旋转模式，缺省为 Trackball
    explicit Camera(CameraMode initialMode = CameraMode::Turntable);

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

    // ==================== 模式专用访问 ====================

    /// Turntable 专用：yaw 角度（仅在 Turntable 模式下有意义）
    double yaw()   const;
    /// Turntable 专用：pitch 角度（仅在 Turntable 模式下有意义）
    double pitch() const;
    /// Turntable 专用：设置 yaw/pitch
    void setYawPitch(double yaw, double pitch);

    /// Trackball 专用：四元数旋转（仅在 Trackball 模式下有意义）
    Quat rotation() const;
    /// Trackball 专用：设置四元数旋转
    void setRotation(const Quat& q);

    // ==================== 交互 ====================

    /// Turntable: delta-based orbit；Trackball: 四元数 delta 旋转
    void orbitDelta(double dx, double dy);

    /// 兼容接口，等效 orbitDelta
    void orbit(double dx, double dy) { orbitDelta(dx, dy); }

    /// Trackball arcball: 开始旋转
    void beginOrbit(int x, int y);
    /// Trackball arcball: 旋转到指定屏幕坐标
    void orbitToPoint(int x, int y);
    /// Trackball arcball: 结束旋转
    void endOrbit();

    void pan(double dx, double dy);
    void zoom(double delta);
    void fitToBox(const AABB& box, double padding = 1.2);

    // ==================== 速度参数 ====================

    void setOrbitSpeed(double s);
    double orbitSpeed() const;

    void setPanSpeed(double s)   { m_panSpeed = s; }
    void setZoomSpeed(double s)  { m_zoomSpeed = s; }

    double panSpeed()  const { return m_panSpeed; }
    double zoomSpeed() const { return m_zoomSpeed; }

    // ==================== 矩阵计算 ====================

    Vec3 eyePosition() const;
    Mat4 viewMatrix() const;
    Mat4 projectionMatrix() const;
    Mat4 viewProjectionMatrix() const;
    Mat4 rotationMatrix() const;
    Frustum frustum() const;

    // ==================== 方向向量 ====================

    Vec3 forward() const;
    Vec3 right() const;
    Vec3 up() const;

private:
    /// 根据模式创建对应的 RotationMode 实例
    void createRotation(CameraMode mode);

    CameraMode m_mode = CameraMode::Trackball;

    std::unique_ptr<RotationMode> m_active;

    Vec3   m_target   = {0, 0, 0};
    double m_distance = 10.0;

    // 投影参数
    int    m_width    = 800;
    int    m_height   = 600;
    double m_fovY     = 3.14159265358979323846 / 4.0;
    double m_nearZ    = 0.1;
    double m_farZ     = 1000.0;
    bool   m_ortho    = true;
    double m_orthoSize = 5.0;

    // 交互速度
    double m_panSpeed    = 0.003;
    double m_zoomSpeed   = 1.08;
    double m_minDistance = 0.001;
};

} // namespace MulanGeo::Engine
