/**
 * @file GLSwapChain.cpp
 * @brief OpenGL 交换链实现
 * @author terry
 * @date 2026-04-16
 */

#include "GLSwapChain.h"

#include <cstdio>

namespace MulanGeo::Engine {

GLSwapChain::GLSwapChain(const SwapChainDesc& desc,
                         const InitParams&    params,
                         const RenderConfig&  renderConfig)
    : m_desc(desc)
    , m_renderConfig(renderConfig)
#ifdef _WIN32
    , m_hdc(params.hdc)
    , m_hwnd(params.hwnd)
#endif
{
    // 设置初始视口
    glViewport(0, 0, static_cast<GLsizei>(desc.width), static_cast<GLsizei>(desc.height));
}

// ----------------------------------------------------------------
// present — 交换前后缓冲区
// ----------------------------------------------------------------

void GLSwapChain::present() {
#ifdef _WIN32
    if (m_hdc) {
        if (m_desc.vsync) {
            // vsync 由像素格式或 WGL_EXT_swap_control 控制
            // 这里简单通过 SwapBuffers 让驱动决定
        }
        SwapBuffers(m_hdc);
    }
#endif
}

// ----------------------------------------------------------------
// resize — 更新视口（OpenGL 默认 FBO 随窗口自动调整，无需重建）
// ----------------------------------------------------------------

void GLSwapChain::resize(uint32_t width, uint32_t height) {
    m_desc.width  = width;
    m_desc.height = height;
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

// ----------------------------------------------------------------
// beginRenderPass — 绑定默认 FBO，清除缓冲区
// ----------------------------------------------------------------

void GLSwapChain::beginRenderPass(CommandList* /*cmd*/) {
    // 绑定默认 Framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0,
               static_cast<GLsizei>(m_desc.width),
               static_cast<GLsizei>(m_desc.height));

    // 确保 writemask 全开，否则 glClear 可能不写深度
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    const auto& cc = m_renderConfig.clearColor;
    glClearColor(cc[0], cc[1], cc[2], cc[3]);
    glClearDepthf(1.0f);

    GLbitfield clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    if (m_renderConfig.stencilBuffer) {
        glClearStencil(0);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }
    glClear(clearMask);
}

// ----------------------------------------------------------------
// endRenderPass — OpenGL 无 render pass 概念，flush 即可
// ----------------------------------------------------------------

void GLSwapChain::endRenderPass(CommandList* /*cmd*/) {
    glFlush();
}

} // namespace MulanGeo::Engine
