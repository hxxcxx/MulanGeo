/**
 * @file Frustum.h
 * @brief 视锥体裁剪平面，用于可见性剔除
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../Math/Vec3.h"
#include "../Math/Mat4.h"
#include "../Math/AABB.h"

namespace MulanGeo::Engine {

struct Plane {
    Vec3  normal = {0, 1, 0};
    double distance = 0;   // n·x + d = 0

    // 点到平面的带符号距离（正=前方，负=后方）
    double signedDistance(const Vec3& point) const;
};

struct Frustum {
    Plane planes[6]; // Left, Right, Bottom, Top, Near, Far

    // 从 viewProjection 矩阵提取裁剪平面
    static Frustum fromViewProjection(const Mat4& vp);

    // 测试点是否在视锥体内
    bool contains(const Vec3& point) const;

    // 测试 AABB 与视锥体是否相交（包含也算相交）
    bool intersects(const AABB& box) const;
};

} // namespace MulanGeo::Engine
