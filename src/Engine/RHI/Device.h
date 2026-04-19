/**
 * @file Device.h
 * @brief RHI设备基类，GPU资源创建入口与帧循环接口
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "Buffer.h"
#include "CommandList.h"
#include "Fence.h"
#include "PipelineState.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "../Math/Mat4.h"
#include "../Window.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <memory>

namespace MulanGeo::Engine {

// ============================================================
// 后端类型
// ============================================================

enum class GraphicsBackend : uint8_t {
    OpenGL,
    Vulkan,
    D3D12,
    D3D11,
};

// ============================================================
// 设备能力信息（后端初始化后填充）
// ============================================================

struct DeviceCapabilities {
    GraphicsBackend backend = GraphicsBackend::OpenGL;
    uint32_t maxTextureSize    = 0;
    uint32_t maxTextureAniso   = 0;
    bool     depthClamp        = false;
    bool     geometryShader    = false;
    bool     tessellationShader = false;
    bool     computeShader     = false;
};

// ============================================================
// 设备创建参数 — 跨后端通用
// ============================================================

struct DeviceCreateInfo {
    GraphicsBackend   backend          = GraphicsBackend::Vulkan;
    NativeWindowHandle window           = {};
    RenderConfig       renderConfig     = {};
    bool               enableValidation = true;
    const char*        appName          = "MulanGeo";
};

// ============================================================
// RHI 设备基类
//
// 所有 GPU 资源的工厂 + 帧循环管理。
// 后端实现继承此类，UI 层只依赖此接口。
// ============================================================

class RHIDevice {
public:
    virtual ~RHIDevice() = default;

    // --- 工厂函数（根据 backend 创建具体实现）---
    static std::unique_ptr<RHIDevice> create(const DeviceCreateInfo& ci);

    // --- 设备信息 ---

    virtual GraphicsBackend backend() const = 0;
    virtual const DeviceCapabilities& capabilities() const = 0;
    virtual const RenderConfig& renderConfig() const = 0;

    // --- 裁剪空间修正 ---
    // 各后端 NDC 约定不同（Vulkan: Y↓ z∈[0,1]，OpenGL: Y↑ z∈[-1,1]）。
    // Camera / Mat4 统一生成标准右手坐标（Y↑ z∈[-1,1]），
    // 由后端提供修正矩阵：finalProj = clipCorrection * projection。
    // 上层无需关心后端差异。

    virtual Mat4 clipSpaceCorrectionMatrix() const = 0;

    // --- 资源创建 ---

    virtual Buffer*         createBuffer(const BufferDesc& desc) = 0;
    virtual Texture*        createTexture(const TextureDesc& desc) = 0;
    virtual Shader*         createShader(const ShaderDesc& desc) = 0;
    virtual PipelineState*  createPipelineState(const GraphicsPipelineDesc& desc) = 0;
    virtual CommandList*    createCommandList() = 0;
    virtual SwapChain*      createSwapChain(const SwapChainDesc& desc) = 0;
    virtual RenderTarget*   createRenderTarget(const RenderTargetDesc& desc) = 0;
    virtual Fence*          createFence(uint64_t initialValue = 0) = 0;

    // --- 资源销毁 ---

    virtual void destroy(Buffer* resource) = 0;
    virtual void destroy(Texture* resource) = 0;
    virtual void destroy(Shader* resource) = 0;
    virtual void destroy(PipelineState* resource) = 0;
    virtual void destroy(CommandList* resource) = 0;
    virtual void destroy(SwapChain* resource) = 0;
    virtual void destroy(RenderTarget* resource) = 0;
    virtual void destroy(Fence* resource) = 0;

    // --- 提交命令 ---

    virtual void executeCommandLists(CommandList** cmdLists,
                                     uint32_t count,
                                     Fence* fence = nullptr,
                                     uint64_t fenceValue = 0) = 0;

    void executeCommandList(CommandList* cmdList,
                            Fence* fence = nullptr,
                            uint64_t fenceValue = 0) {
        executeCommandLists(&cmdList, 1, fence, fenceValue);
    }

    // --- 等待 GPU 空闲 ---

    virtual void waitIdle() = 0;

    // ============================================================
    // 帧循环接口（UI 层的标准渲染流程）
    //
    //   device->beginFrame();
    //   auto* cmd = device->frameCommandList();
    //   cmd->begin();
    //   swapchain->beginRenderPass(cmd);
    //   // ... draw calls ...
    //   swapchain->endRenderPass(cmd);
    //   cmd->end();
    //   device->submitAndPresent(swapchain);
    // ============================================================

    /// 每帧开头：等待上一轮完成、acquire next image、重置资源
    virtual void beginFrame() = 0;

    /// 获取当前帧的 CommandList（已 reset，可直接 begin）
    virtual CommandList* frameCommandList() = 0;

    /// 提交当前帧命令 + present
    virtual void submitAndPresent(SwapChain* swapchain) = 0;

    /// 提交当前帧命令（无 present — 用于离屏渲染）
    virtual void submitOffscreen() = 0;

    // ============================================================
    // Descriptor 绑定（UBO / Texture 统一绑定接口）
    // ============================================================

    /// 将一组 UBO 绑定到当前管线（VK: allocate descriptor set + bind）
    struct UniformBufferBind {
        uint32_t binding;
        Buffer*  buffer;
        uint32_t offset;
        uint32_t size;
    };

    virtual void bindUniformBuffers(CommandList* cmd, PipelineState* pso,
                                    const UniformBufferBind* binds,
                                    uint32_t count) = 0;

protected:
    RHIDevice() = default;
    RHIDevice(const RHIDevice&) = delete;
    RHIDevice& operator=(const RHIDevice&) = delete;
};

// ============================================================
// RAII 资源指针 — 自动调用 device->destroy()
// ============================================================

struct DeviceResourceDeleter {
    RHIDevice* device = nullptr;
    template <typename T>
    void operator()(T* ptr) const {
        if (ptr && device) device->destroy(ptr);
    }
};

template <typename T>
using ResourcePtr = std::unique_ptr<T, DeviceResourceDeleter>;

/// 创建 RAII 资源指针
template <typename T>
ResourcePtr<T> makeResource(T* raw, RHIDevice* device) {
    return ResourcePtr<T>(raw, DeviceResourceDeleter{device});
}

} // namespace MulanGeo::Engine
