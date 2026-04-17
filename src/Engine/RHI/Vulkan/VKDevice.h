/**
 * @file VKDevice.h
 * @brief Vulkan设备实现，资源工厂与后端入口
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "VkCommon.h"
#include "../Device.h"
#include "../../Window.h"
#include "VkConvert.h"
#include "VKBuffer.h"
#include "VKTexture.h"
#include "VKShader.h"
#include "VKPipelineState.h"
#include "VKCommandList.h"
#include "VKSwapChain.h"
#include "VKRenderTarget.h"
#include "VKFence.h"
#include "VKUploadContext.h"
#include "VKFrameContext.h"
#include "VKDescriptorAllocator.h"

#include <vector>
#include <memory>
#include <set>

namespace MulanGeo::Engine {

class VKDevice : public RHIDevice {
public:
    struct CreateInfo {
        bool               enableValidation = true;
        NativeWindowHandle window;
        RenderConfig       renderConfig;
        uint32_t           frameCount       = 0;
        const char*        appName          = "MulanGeo";
    };

    explicit VKDevice(const CreateInfo& ci = {});
    explicit VKDevice(const DeviceCreateInfo& ci);
    ~VKDevice();

    // --- Device 信息 ---
    GraphicsBackend backend() const override;
    const DeviceCapabilities& capabilities() const override;
    const RenderConfig& renderConfig() const override;

    // --- 资源创建 ---
    Buffer*         createBuffer(const BufferDesc& desc) override;
    Texture*        createTexture(const TextureDesc& desc) override;
    Shader*         createShader(const ShaderDesc& desc) override;
    PipelineState*  createPipelineState(const GraphicsPipelineDesc& desc) override;
    CommandList*    createCommandList() override;
    SwapChain*      createSwapChain(const SwapChainDesc& desc) override;
    RenderTarget*   createRenderTarget(const RenderTargetDesc& desc) override;
    Fence*          createFence(uint64_t initialValue = 0) override;

    // --- 资源销毁 ---
    void destroy(Buffer* resource) override;
    void destroy(Texture* resource) override;
    void destroy(Shader* resource) override;
    void destroy(PipelineState* resource) override;
    void destroy(CommandList* resource) override;
    void destroy(SwapChain* resource) override;
    void destroy(RenderTarget* resource) override;
    void destroy(Fence* resource) override;

    // --- 提交命令 ---
    void executeCommandLists(CommandList** cmdLists, uint32_t count,
                             Fence* fence = nullptr, uint64_t fenceValue = 0) override;
    void waitIdle() override;

    // --- 帧循环 ---
    void beginFrame() override;
    CommandList* frameCommandList() override;
    void submitAndPresent(SwapChain* swapchain) override;
    void submitOffscreen() override;

    // --- Descriptor 绑定 ---
    void bindUniformBuffers(CommandList* cmd, PipelineState* pso,
                            const UniformBufferBind* binds, uint32_t count) override;

    // --- Vulkan 特有访问器 ---
    vk::Instance         vkInstance()          const { return m_instance; }
    vk::PhysicalDevice   vkPhysicalDevice()    const { return m_physicalDevice; }
    vk::Device           vkDevice()            const { return m_device; }
    vk::Queue            graphicsQueue()       const { return m_graphicsQueue; }
    uint32_t             graphicsQueueFamily() const { return m_graphicsQueueFamily; }
    VmaAllocator         vmaAllocator()        const { return m_allocator; }

    VKUploadContext&       uploadContext()       { return *m_uploadContext; }
    VKDescriptorAllocator& descriptorAllocator() { return *m_descriptorAllocator; }
    VKFrameContext&        currentFrameContext() { return *m_frameContexts[m_currentFrame]; }
    uint32_t               currentFrameIndex()   const { return m_currentFrame; }

private:
    void init(const CreateInfo& ci);
    void shutdown();
    void pickPhysicalDevice(const std::vector<vk::PhysicalDevice>& devices);
    void createLogicalDevice(bool enableValidation);
    vk::SurfaceKHR createSurface(const NativeWindowHandle& window);
    void initFrameContexts(uint32_t count);

    // --- Vulkan 核心 ---
    vk::Instance                m_instance;
    vk::PhysicalDevice          m_physicalDevice;
    vk::Device                  m_device;
    vk::SurfaceKHR              m_surface;
    vk::Queue                   m_graphicsQueue;
    vk::Queue                   m_presentQueue;
    VmaAllocator                m_allocator = nullptr;

    uint32_t                    m_graphicsQueueFamily = 0;
    uint32_t                    m_presentQueueFamily  = 0;
    NativeWindowHandle          m_nativeWindow;
    RenderConfig                m_renderConfig;

    DeviceCapabilities          m_caps;
    std::vector<VKSwapChain*>   m_swapChains;

    // --- 私有组件 ---
    std::unique_ptr<VKUploadContext>             m_uploadContext;
    std::vector<std::unique_ptr<VKFrameContext>> m_frameContexts;
    std::unique_ptr<VKDescriptorAllocator>       m_descriptorAllocator;
    std::unique_ptr<VKCommandList>               m_frameCmdList;

    uint32_t                    m_frameCount   = 2;
    uint32_t                    m_currentFrame = 0;
};

} // namespace MulanGeo::Engine
