/**
 * @file GLDevice.h
 * @brief OpenGL 设备实现，资源工厂与后端入口
 * @author terry
 * @date 2026-04-16
 */

#pragma once

#include "GLCommon.h"
#include "../Device.h"
#include "../../Window.h"

#include <vector>
#include <memory>
#include <cstdio>

namespace MulanGeo::Engine {

// ── 前向声明 OpenGL 子资源类型（后续实现）──
class GLBuffer;
class GLTexture;
class GLShader;
class GLPipelineState;
class GLCommandList;
class GLSwapChain;
class GLFence;

class GLDevice : public RHIDevice {
public:
    struct CreateInfo {
        bool               enableValidation = true;
        NativeWindowHandle window;
        RenderConfig       renderConfig;
        const char*        appName = "MulanGeo";
    };

    explicit GLDevice(const CreateInfo& ci) {
        init(ci);
    }

    /// 从通用 DeviceCreateInfo 构造（供 RHIDevice::create 工厂调用）
    explicit GLDevice(const DeviceCreateInfo& ci) {
        CreateInfo glCI;
        glCI.enableValidation = ci.enableValidation;
        glCI.window           = ci.window;
        glCI.renderConfig     = ci.renderConfig;
        glCI.appName          = ci.appName;
        init(glCI);
    }

    ~GLDevice();

    // --- Device 信息 ---

    GraphicsBackend backend() const override {
        return GraphicsBackend::OpenGL;
    }

    const DeviceCapabilities& capabilities() const override {
        return m_caps;
    }

    const RenderConfig& renderConfig() const override {
        return m_renderConfig;
    }

    // --- 资源创建（桩实现，后续补全）---

    Buffer*        createBuffer(const BufferDesc& desc) override;
    Texture*       createTexture(const TextureDesc& desc) override;
    Shader*        createShader(const ShaderDesc& desc) override;
    PipelineState* createPipelineState(const GraphicsPipelineDesc& desc) override;
    CommandList*   createCommandList() override;
    SwapChain*     createSwapChain(const SwapChainDesc& desc) override;
    Fence*         createFence(uint64_t initialValue = 0) override;

    // --- 资源销毁 ---

    void destroy(Buffer* resource) override;
    void destroy(Texture* resource) override;
    void destroy(Shader* resource) override;
    void destroy(PipelineState* resource) override;
    void destroy(CommandList* resource) override;
    void destroy(SwapChain* resource) override;
    void destroy(Fence* resource) override;

    // --- 提交命令 ---

    void executeCommandLists(CommandList** cmdLists,
                             uint32_t count,
                             Fence* fence = nullptr,
                             uint64_t fenceValue = 0) override;

    void waitIdle() override;

    // --- 帧循环 ---

    void beginFrame() override;
    CommandList* frameCommandList() override;
    void submitAndPresent(SwapChain* swapchain) override;

    // --- Descriptor 绑定 ---

    void bindUniformBuffers(CommandList* cmd, PipelineState* pso,
                            const UniformBufferBind* binds,
                            uint32_t count) override;

    // --- OpenGL 特有访问器 ---

#ifdef _WIN32
    HDC   hdc()   const { return m_hdc; }
    HGLRC hglrc() const { return m_hglrc; }
#endif

    bool isInitialized() const { return m_initialized; }

private:
    void init(const CreateInfo& ci);
    void shutdown();
    void queryCapabilities();

#ifdef _WIN32
    bool createWGLContext(HWND hwnd, bool enableValidation);
    HDC   m_hdc   = nullptr;
    HGLRC m_hglrc = nullptr;
    HWND  m_hwnd  = nullptr;
#endif

    bool               m_initialized = false;
    NativeWindowHandle m_nativeWindow;
    RenderConfig       m_renderConfig;
    DeviceCapabilities m_caps;
};

} // namespace MulanGeo::Engine
