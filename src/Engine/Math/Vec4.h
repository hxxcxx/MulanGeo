/**
 * @file Vec4.h
 * @brief 四维向量，用于齐次坐标与颜色表示
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

namespace MulanGeo::Engine {

struct Vec4 {
    double x = 0, y = 0, z = 0, w = 0;

    static constexpr Vec4 zero() { return {0, 0, 0, 0}; }

    constexpr Vec4 operator+(const Vec4& b) const { return {x+b.x, y+b.y, z+b.z, w+b.w}; }
    constexpr Vec4 operator*(double s) const { return {x*s, y*s, z*s, w*s}; }
    constexpr Vec4 operator-() const { return {-x, -y, -z, -w}; }

    constexpr bool operator==(const Vec4& b) const {
        return x==b.x && y==b.y && z==b.z && w==b.w;
    }
};

inline constexpr Vec4 operator*(double s, const Vec4& v) { return v * s; }

} // namespace MulanGeo::Engine
