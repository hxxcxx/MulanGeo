/*
 * RHI 设备 — 资源工厂与后端入口
 *
 * 定义 RHIDevice 基类，所有 GPU 资源的创建入口。
 * 每个后端（GL/VK/D3D12）提供自己的实现。
 *
 * 设计原则：
 * - Device 只负责创建资源，不负责绑定或绘制
 * - 绑定和绘制由 CommandList 负责
 * - SwapChain 由 Device 创建，管理前后缓冲
 */

#pragma once

#include "Buffer.h"
#include "CommandList.h"
#include "Fence.h"
#include "PipelineState.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"

#include <cstdint>
#include <string_view>

namespace MulanGeo::Engine {

// ============================================================
// 后端类型
// ============================================================

enum class GraphicsBackend : uint8_t {
    OpenGL,
    Vulkan,
    D3D12,
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
// RHI 设备基类
//
// 所有 GPU 资源的工厂。后端实现继承此类。
// ============================================================

class RHIDevice {
public:
    virtual ~RHIDevice() = default;

    // --- 设备信息 ---

    virtual GraphicsBackend backend() const = 0;
    virtual const DeviceCapabilities& capabilities() const = 0;

    // --- 资源创建 ---

    virtual Buffer*         createBuffer(const BufferDesc& desc) = 0;
    virtual Texture*        createTexture(const TextureDesc& desc) = 0;
    virtual Shader*         createShader(const ShaderDesc& desc) = 0;
    virtual PipelineState*  createPipelineState(const GraphicsPipelineDesc& desc) = 0;
    virtual CommandList*    createCommandList() = 0;
    virtual SwapChain*      createSwapChain(const SwapChainDesc& desc) = 0;
    virtual Fence*          createFence(uint64_t initialValue = 0) = 0;

    // --- 资源销毁 ---

    virtual void destroy(Buffer* resource) = 0;
    virtual void destroy(Texture* resource) = 0;
    virtual void destroy(Shader* resource) = 0;
    virtual void destroy(PipelineState* resource) = 0;
    virtual void destroy(CommandList* resource) = 0;
    virtual void destroy(SwapChain* resource) = 0;
    virtual void destroy(Fence* resource) = 0;

    // --- 提交命令 ---

    virtual void executeCommandLists(CommandList** cmdLists,
                                     uint32_t count,
                                     Fence* fence = nullptr,
                                     uint64_t fenceValue = 0) = 0;

    // 单个 CommandList 的便捷接口
    void executeCommandList(CommandList* cmdList,
                            Fence* fence = nullptr,
                            uint64_t fenceValue = 0) {
        executeCommandLists(&cmdList, 1, fence, fenceValue);
    }

    // --- 等待 GPU 空闲 ---

    virtual void waitIdle() = 0;

protected:
    RHIDevice() = default;
    RHIDevice(const RHIDevice&) = delete;
    RHIDevice& operator=(const RHIDevice&) = delete;
};

} // namespace MulanGeo::Engine
