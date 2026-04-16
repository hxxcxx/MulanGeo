/**
 * @file Vec3.h
 * @brief 三维向量，基础几何运算
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include <cmath>
#include <cstdint>

namespace MulanGeo::Engine {

struct Vec3 {
    double x = 0, y = 0, z = 0;

    static constexpr Vec3 zero()   { return {0, 0, 0}; }
    static constexpr Vec3 one()    { return {1, 1, 1}; }
    static constexpr Vec3 unitX()  { return {1, 0, 0}; }
    static constexpr Vec3 unitY()  { return {0, 1, 0}; }
    static constexpr Vec3 unitZ()  { return {0, 0, 1}; }

    // --- 运算符 ---

    constexpr Vec3 operator+(const Vec3& b) const { return {x+b.x, y+b.y, z+b.z}; }
    constexpr Vec3 operator-(const Vec3& b) const { return {x-b.x, y-b.y, z-b.z}; }
    constexpr Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
    constexpr Vec3 operator/(double s) const { return {x/s, y/s, z/s}; }
    constexpr Vec3 operator-()       const { return {-x, -y, -z}; }
    constexpr Vec3 operator*(const Vec3& b) const { return {x*b.x, y*b.y, z*b.z}; } // 逐分量

    constexpr Vec3& operator+=(const Vec3& b) { x+=b.x; y+=b.y; z+=b.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& b) { x-=b.x; y-=b.y; z-=b.z; return *this; }
    constexpr Vec3& operator*=(double s) { x*=s; y*=s; z*=s; return *this; }

    constexpr bool operator==(const Vec3& b) const { return x==b.x && y==b.y && z==b.z; }

    // --- 几何运算 ---

    double length() const { return std::sqrt(x*x + y*y + z*z); }
    constexpr double lengthSq() const { return x*x + y*y + z*z; }

    Vec3 normalized() const {
        double len = length();
        return len > 0 ? *this / len : zero();
    }

    void normalize() { *this = normalized(); }

    // --- 全局函数 ---

    static constexpr double dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    static constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x
        };
    }

    static double distance(const Vec3& a, const Vec3& b) {
        return (a - b).length();
    }

    static constexpr Vec3 lerp(const Vec3& a, const Vec3& b, double t) {
        return a + (b - a) * t;
    }

    static constexpr Vec3 min(const Vec3& a, const Vec3& b) {
        return {a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z};
    }

    static constexpr Vec3 max(const Vec3& a, const Vec3& b) {
        return {a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z};
    }
};

// 标量 * 向量
inline constexpr Vec3 operator*(double s, const Vec3& v) { return v * s; }

} // namespace MulanGeo::Engine
