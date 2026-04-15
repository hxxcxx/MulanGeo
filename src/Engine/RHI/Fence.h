/*
 * 栅栏 — CPU/GPU 同步原语
 *
 * 用于渲染线程与主线程之间的同步，
 * 以及 CommandList 提交后等待 GPU 完成。
 */

#pragma once

#include <cstdint>

namespace MulanGeo::Engine {

class Fence {
public:
    virtual ~Fence() = default;

    // CPU 端发起信号（递增内部计数器）
    virtual void signal(uint64_t value) = 0;

    // CPU 端阻塞等待，直到 GPU 达到指定值
    virtual void wait(uint64_t value) = 0;

    // 查询当前 GPU 已完成的值
    virtual uint64_t completedValue() const = 0;

protected:
    Fence() = default;
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
};

} // namespace MulanGeo::Engine
