/**
 * @file Mat4.h
 * @brief 4x4列主序矩阵，提供投影、变换等运算
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "Vec3.h"
#include "Vec4.h"

#include <cmath>
#include <cstring>

namespace MulanGeo::Engine {

struct Mat4 {
    // 列主序：m[col][row]，即 m[0]={c0r0,c0r1,c0r2,c0r3}
    double m[4][4] = {
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0,0,1}
    };

    static constexpr Mat4 identity() { return {}; }

    // --- 按列访问 ---

    constexpr Vec4 col(int c) const { return {m[c][0], m[c][1], m[c][2], m[c][3]}; }
    constexpr void setCol(int c, const Vec4& v) {
        m[c][0]=v.x; m[c][1]=v.y; m[c][2]=v.z; m[c][3]=v.w;
    }

    // --- 矩阵乘法 ---

    constexpr Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                r.m[c][row] =
                    m[0][row] * b.m[c][0] +
                    m[1][row] * b.m[c][1] +
                    m[2][row] * b.m[c][2] +
                    m[3][row] * b.m[c][3];
            }
        }
        return r;
    }

    constexpr Mat4& operator*=(const Mat4& b) {
        *this = *this * b;
        return *this;
    }

    // --- 矩阵 × 向量 ---

    constexpr Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]*v.w,
            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]*v.w,
            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]*v.w,
            m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]*v.w,
        };
    }

    // 变换点（w=1）
    constexpr Vec3 transformPoint(const Vec3& p) const {
        Vec4 r = *this * Vec4{p.x, p.y, p.z, 1.0};
        if (r.w != 0 && r.w != 1) { r.x/=r.w; r.y/=r.w; r.z/=r.w; }
        return {r.x, r.y, r.z};
    }

    // 变换方向（w=0，不受平移影响）
    constexpr Vec3 transformDir(const Vec3& d) const {
        Vec4 r = *this * Vec4{d.x, d.y, d.z, 0.0};
        return {r.x, r.y, r.z};
    }

    // --- 转置 ---

    constexpr Mat4 transposed() const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row)
                r.m[c][row] = m[row][c];
        return r;
    }

    // --- 逆矩阵（伴随矩阵法）---

    Mat4 inverted() const;

    // --- 工厂：变换 ---

    static Mat4 translation(const Vec3& t) {
        Mat4 r;
        r.m[3][0] = t.x;
        r.m[3][1] = t.y;
        r.m[3][2] = t.z;
        return r;
    }

    static Mat4 scaling(const Vec3& s) {
        Mat4 r;
        r.m[0][0] = s.x;
        r.m[1][1] = s.y;
        r.m[2][2] = s.z;
        return r;
    }

    static Mat4 scaling(double s) { return scaling({s, s, s}); }

    // 绕任意轴旋转（弧度）
    static Mat4 rotationAxis(const Vec3& axis, double radians);

    // 绕坐标轴旋转
    static Mat4 rotationX(double radians);
    static Mat4 rotationY(double radians);
    static Mat4 rotationZ(double radians);

    // --- 工厂：视图 / 投影 ---

    // 右手坐标系 lookAt
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

    // 透视投影（右手，z ∈ [-1, 1]）
    // fovY 为垂直视场角（弧度）
    static Mat4 perspective(double fovY, double aspect, double nearZ, double farZ);

    // 正交投影（右手，z ∈ [-1, 1]）
    static Mat4 ortho(double left, double right, double bottom, double top,
                      double nearZ, double farZ);

    // --- 数据访问 ---

    constexpr const double* data() const { return &m[0][0]; }
    constexpr double* data() { return &m[0][0]; }
};

} // namespace MulanGeo::Engine
