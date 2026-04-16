/**
 * @file Quat.h
 * @brief 四元数旋转表示，支持球面插值与轴角转换
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "Vec3.h"
#include "Mat4.h"

#include <cmath>

namespace MulanGeo::Engine {

struct Quat {
    double x = 0, y = 0, z = 0, w = 1;  // w 为标量部分

    static constexpr Quat identity() { return {0, 0, 0, 1}; }

    // --- 运算符 ---

    constexpr Quat operator*(const Quat& b) const {
        return {
            w*b.x + x*b.w + y*b.z - z*b.y,
            w*b.y - x*b.z + y*b.w + z*b.x,
            w*b.z + x*b.y - y*b.x + z*b.w,
            w*b.w - x*b.x - y*b.y - z*b.z,
        };
    }

    constexpr bool operator==(const Quat& b) const {
        return x==b.x && y==b.y && z==b.z && w==b.w;
    }

    // --- 属性 ---

    double length() const { return std::sqrt(x*x + y*y + z*z + w*w); }

    Quat normalized() const {
        double len = length();
        return len > 0 ? Quat{x/len, y/len, z/len, w/len} : identity();
    }

    constexpr Quat conjugate() const { return {-x, -y, -z, w}; }

    // --- 工厂 ---

    // 从轴角创建（弧度）
    static Quat fromAxisAngle(const Vec3& axis, double radians) {
        double half = radians * 0.5;
        double s = std::sin(half);
        Vec3 n = axis.normalized();
        return {n.x*s, n.y*s, n.z*s, std::cos(half)};
    }

    // 从 Euler 角创建（弧度，YXZ 顺序）
    static Quat fromEuler(double yaw, double pitch, double roll) {
        double cy = std::cos(yaw*0.5),   sy = std::sin(yaw*0.5);
        double cp = std::cos(pitch*0.5), sp = std::sin(pitch*0.5);
        double cr = std::cos(roll*0.5),  sr = std::sin(roll*0.5);
        return {
            cy*sp*cr + sy*cp*sr,
            cy*cp*sr - sy*sp*cr,
            sy*cp*cr - cy*sp*sr,
            cy*cp*cr + sy*sp*sr,
        };
    }

    // 从两个方向向量创建旋转
    static Quat fromToRotation(const Vec3& from, const Vec3& to) {
        double d = Vec3::dot(from, to);
        if (d >= 1.0) return identity();
        if (d <= -1.0) {
            Vec3 axis = Vec3::cross(Vec3::unitX(), from);
            if (axis.lengthSq() < 1e-6) axis = Vec3::cross(Vec3::unitY(), from);
            return fromAxisAngle(axis.normalized(), 3.141592653589793);
        }
        double s = std::sqrt((1.0 + d) * 2.0);
        Vec3 c = Vec3::cross(from, to);
        return {c.x/s, c.y/s, c.z/s, s*0.5};
    }

    // --- 转换 ---

    // 转为旋转矩阵
    Mat4 toMat4() const {
        Quat n = normalized();
        double xx = n.x*n.x, yy = n.y*n.y, zz = n.z*n.z;
        double xy = n.x*n.y, xz = n.x*n.z, yz = n.y*n.z;
        double wx = n.w*n.x, wy = n.w*n.y, wz = n.w*n.z;

        Mat4 r;
        r.m[0][0] = 1-2*(yy+zz); r.m[1][0] = 2*(xy-wz);   r.m[2][0] = 2*(xz+wy);
        r.m[0][1] = 2*(xy+wz);   r.m[1][1] = 1-2*(xx+zz);  r.m[2][1] = 2*(yz-wx);
        r.m[0][2] = 2*(xz-wy);   r.m[1][2] = 2*(yz+wx);    r.m[2][2] = 1-2*(xx+yy);
        return r;
    }

    // --- 插值 ---

    static Quat slerp(const Quat& a, const Quat& b, double t) {
        double dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        Quat qb = b;
        if (dot < 0) { qb = {-b.x, -b.y, -b.z, -b.w}; dot = -dot; }

        if (dot > 0.9995) {
            // 接近共线，用线性插值
            return Quat{
                a.x + t*(qb.x-a.x),
                a.y + t*(qb.y-a.y),
                a.z + t*(qb.z-a.z),
                a.w + t*(qb.w-a.w),
            }.normalized();
        }

        double theta = std::acos(dot);
        double sinTheta = std::sin(theta);
        double wa = std::sin((1-t)*theta) / sinTheta;
        double wb = std::sin(t*theta) / sinTheta;
        return {
            wa*a.x + wb*qb.x,
            wa*a.y + wb*qb.y,
            wa*a.z + wb*qb.z,
            wa*a.w + wb*qb.w,
        };
    }
};

} // namespace MulanGeo::Engine
