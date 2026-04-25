/**
 * @file RotationMode.h
 * @brief 相机旋转模式抽象接口 — Strategy 模式
 *
 * 定义了旋转模式必须实现的操作：
 *  - 方向向量（forward / right / up）
 *  - 轨道旋转（orbitDelta）
 *  - Arcball 交互生命周期（begin / orbitToPoint / end）
 *  - 速度参数
 *
 * @author hxxcxx
 * @date 2026-04-25
 */

#pragma once

#include "../../Math/Math.h"

namespace MulanGeo::Engine {

class RotationMode {
public:
    virtual ~RotationMode() = default;

    virtual Vec3 forward() const = 0;
    virtual Vec3 right()   const = 0;
    virtual Vec3 up()      const = 0;

    virtual void orbitDelta(double dx, double dy) = 0;

    virtual void beginOrbit(int x, int y, int viewW, int viewH) {}
    virtual void orbitToPoint(int x, int y, int viewW, int viewH) {}
    virtual void endOrbit() {}

    virtual void setOrbitSpeed(double s) = 0;
    virtual double orbitSpeed() const = 0;
};

} // namespace MulanGeo::Engine
