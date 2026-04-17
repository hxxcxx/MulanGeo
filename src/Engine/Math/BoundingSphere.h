/**
 * @file BoundingSphere.h
 * @brief 包围球，用于快速剔除与碰撞检测
 * @author hxxcxx
 * @date 2026-04-17
 */

#pragma once

#include "Vec3.h"
#include "AABB.h"
#include "Mat4.h"

#include <cmath>
#include <algorithm>

namespace MulanGeo::Engine {

struct BoundingSphere {
    Vec3   center;
    double radius = -1.0;  // 负值表示无效

    // --- 工厂 ---

    static BoundingSphere invalid() { return {}; }

    static BoundingSphere fromCenterRadius(const Vec3& c, double r) {
        return {c, r};
    }

    /// 从 AABB 计算外接球
    static BoundingSphere fromAABB(const AABB& box) {
        if (box.isEmpty()) return invalid();
        Vec3 c = box.center();
        return {c, Vec3::distance(c, box.max)};
    }

    // --- 状态 ---

    bool isValid() const { return radius >= 0.0; }

    void reset() { center = Vec3::zero(); radius = -1.0; }

    // --- 扩展 ---

    /// 扩展以包含一个点
    void expand(const Vec3& point) {
        if (!isValid()) {
            center = point;
            radius = 0.0;
            return;
        }
        Vec3 diff = point - center;
        double dist = diff.length();
        if (dist <= radius) return;   // 已包含

        // Ritter 更新：中心沿 diff 方向偏移
        double newRadius = (radius + dist) * 0.5;
        double shift = newRadius - radius;
        center += diff * (shift / dist);
        radius = newRadius;
    }

    /// 扩展以包含另一球
    void expand(const BoundingSphere& other) {
        if (!other.isValid()) return;
        if (!isValid()) { *this = other; return; }

        Vec3 diff  = other.center - center;
        double dist = diff.length();

        // 当前球完全包含 other
        if (dist + other.radius <= radius) return;
        // other 完全包含当前球
        if (dist + radius <= other.radius) { *this = other; return; }

        double newRadius = (radius + dist + other.radius) * 0.5;
        if (dist > 1e-12) {
            center += diff * ((newRadius - radius) / dist);
        }
        radius = newRadius;
    }

    /// 扩展以包含 AABB
    void expand(const AABB& box) {
        if (box.isEmpty()) return;
        expand(BoundingSphere::fromAABB(box));
    }

    // --- 查询 ---

    bool contains(const Vec3& point) const {
        if (!isValid()) return false;
        return (point - center).lengthSq() <= radius * radius;
    }

    bool intersects(const BoundingSphere& other) const {
        if (!isValid() || !other.isValid()) return false;
        double r = radius + other.radius;
        return (center - other.center).lengthSq() <= r * r;
    }

    bool intersects(const AABB& box) const {
        if (!isValid() || box.isEmpty()) return false;
        // 最近点测试
        Vec3 closest = Vec3::max(box.min, Vec3::min(box.max, center));
        return (closest - center).lengthSq() <= radius * radius;
    }

    // --- 变换 ---

    /// 用矩阵变换包围球（假定等比缩放，取 X 列长度作为缩放因子）
    BoundingSphere transformed(const Mat4& m) const {
        if (!isValid()) return invalid();
        Vec3 newCenter = m.transformPoint(center);
        // 从矩阵提取近似缩放：取三列长度的最大值
        double sx = Vec3{m.m[0][0], m.m[0][1], m.m[0][2]}.length();
        double sy = Vec3{m.m[1][0], m.m[1][1], m.m[1][2]}.length();
        double sz = Vec3{m.m[2][0], m.m[2][1], m.m[2][2]}.length();
        double maxScale = std::max({sx, sy, sz});
        return {newCenter, radius * maxScale};
    }
};

} // namespace MulanGeo::Engine
