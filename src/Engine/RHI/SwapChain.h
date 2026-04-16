/**
 * @file SwapChain.h
 * @brief 交换链，前后缓冲区管理与呈现
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "Texture.h"

#include <cstdint>

namespace MulanGeo::Engine {

// ============================================================
// 交换链描述
// ============================================================

struct SwapChainDesc {
    uint32_t       width       = 0;
    uint32_t       height      = 0;
    TextureFormat  format      = TextureFormat::RGBA8_UNorm;
    uint32_t       bufferCount = 2;       // 双缓冲 / 三缓冲
    bool           vsync       = true;
};

// ============================================================
// 交换链基类
// ============================================================

class SwapChain {
public:
    virtual ~SwapChain() = default;

    virtual const SwapChainDesc& desc() const = 0;

    // 获取当前后缓冲（作为 RenderTarget 绑定）
    virtual Texture* currentBackBuffer() = 0;

    // 呈现
    virtual void present() = 0;

    // 窗口大小变化时调用
    virtual void resize(uint32_t width, uint32_t height) = 0;

    // --- RenderPass（后端实现，VK 有真实的 renderPass，GL 是 clear + noop）---

    /// 开始 render pass：清除 color + depth，绑定 framebuffer
    virtual void beginRenderPass(CommandList* cmd) = 0;

    /// 结束 render pass
    virtual void endRenderPass(CommandList* cmd) = 0;

    // 便捷查询
    uint32_t width()  const { return desc().width; }
    uint32_t height() const { return desc().height; }

protected:
    SwapChain() = default;
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;
};

} // namespace MulanGeo::Engine
