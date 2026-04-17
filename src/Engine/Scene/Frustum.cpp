#include "Frustum.h"

#include <cmath>

namespace MulanGeo::Engine {

double Plane::signedDistance(const Vec3& point) const {
    return Vec3::dot(normal, point) + distance;
}

Frustum Frustum::fromViewProjection(const Mat4& vp) {
    Frustum f;

    // vp 行
    auto row = [&](int r) -> Vec4 {
        return {vp.m[0][r], vp.m[1][r], vp.m[2][r], vp.m[3][r]};
    };

    auto extract = [&](const Vec4& p) -> Plane {
        double len = std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
        if (len > 0) {
            return { {p.x/len, p.y/len, p.z/len}, p.w/len };
        }
        return { {0,1,0}, 0 };
    };

    // Gribb-Hartmann 方法：从组合矩阵直接提取平面
    // 左: row3 + row0
    f.planes[0] = extract(row(3) + row(0));
    // 右: row3 - row0
    f.planes[1] = extract(row(3) - row(0));
    // 下: row3 + row1
    f.planes[2] = extract(row(3) + row(1));
    // 上: row3 - row1
    f.planes[3] = extract(row(3) - row(1));
    // 近: row3 + row2
    f.planes[4] = extract(row(3) + row(2));
    // 远: row3 - row2
    f.planes[5] = extract(row(3) - row(2));

    return f;
}

bool Frustum::contains(const Vec3& point) const {
    for (int i = 0; i < 6; ++i) {
        if (planes[i].signedDistance(point) < 0)
            return false;
    }
    return true;
}

bool Frustum::intersects(const AABB& box) const {
    for (int i = 0; i < 6; ++i) {
        // AABB 上离平面最远的正角点
        Vec3 p = box.min;
        if (planes[i].normal.x >= 0) p.x = box.max.x;
        if (planes[i].normal.y >= 0) p.y = box.max.y;
        if (planes[i].normal.z >= 0) p.z = box.max.z;

        // 如果最远正角点在平面外侧，整个 AABB 在外侧
        if (planes[i].signedDistance(p) < 0)
            return false;
    }
    return true;
}

} // namespace MulanGeo::Engine
