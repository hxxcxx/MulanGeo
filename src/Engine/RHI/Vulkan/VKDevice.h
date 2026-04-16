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
#include "VKFence.h"
#include "VKUploadContext.h"
#include "VKFrameContext.h"
#include "VKDescriptorAllocator.h"

#include <vector>
#include <memory>
#include <set>
#include <cstdio>

namespace MulanGeo::Engine {

class VKDevice : public RHIDevice {
public:
    struct CreateInfo {
        bool               enableValidation = true;
        NativeWindowHandle window;                  // 跨平台窗口句柄
        RenderConfig       renderConfig;             // 渲染配置
        uint32_t           frameCount       = 0;    // 0 = 自动取 renderConfig.bufferCount
        const char*        appName          = "MulanGeo";
    };

    explicit VKDevice(const CreateInfo& ci = {}) {
        init(ci);
    }

    ~VKDevice() {
        m_device.waitIdle();

        // 先销毁私有组件（依赖 device）
        m_frameContexts.clear();
        m_uploadContext.reset();
        m_descriptorAllocator.reset();
        m_swapChains.clear();

        shutdown();
    }

    // --- Device 信息 ---

    GraphicsBackend backend() const override {
        return GraphicsBackend::Vulkan;
    }

    const DeviceCapabilities& capabilities() const override {
        return m_caps;
    }

    // --- 资源创建 ---

    Buffer* createBuffer(const BufferDesc& desc) override {
        auto* buf = new VKBuffer(desc, m_allocator);
        if (buf->needsUpload()) {
            m_uploadContext->uploadBufferInit(buf);
        }
        return buf;
    }

    Texture* createTexture(const TextureDesc& desc) override {
        return new VKTexture(desc, m_device, m_allocator);
    }

    Shader* createShader(const ShaderDesc& desc) override {
        return new VKShader(desc, m_device);
    }

    PipelineState* createPipelineState(const GraphicsPipelineDesc& desc) override {
        return new VKPipelineState(desc, m_device);
    }

    CommandList* createCommandList() override {
        return new VKCommandList(m_device, m_graphicsQueueFamily);
    }

    SwapChain* createSwapChain(const SwapChainDesc& desc) override {
        VKSwapChain::InitParams params;
        params.instance            = m_instance;
        params.physicalDevice      = m_physicalDevice;
        params.device              = m_device;
        params.allocator           = m_allocator;
        params.graphicsQueueFamily = m_graphicsQueueFamily;
        params.presentQueueFamily  = m_presentQueueFamily;
        params.graphicsQueue       = m_graphicsQueue;
        params.presentQueue        = m_presentQueue;
        params.surface             = m_surface;

        auto* swapchain = new VKSwapChain(desc, params, m_renderConfig);
        m_swapChains.push_back(swapchain);

        // swapchain 创建后初始化帧上下文
        initFrameContexts(m_frameCount);

        return swapchain;
    }

    Fence* createFence(uint64_t initialValue = 0) override {
        return new VKFence(m_device, initialValue);
    }

    // --- 资源销毁 ---

    void destroy(Buffer* resource) override         { delete static_cast<VKBuffer*>(resource); }
    void destroy(Texture* resource) override        { delete static_cast<VKTexture*>(resource); }
    void destroy(Shader* resource) override         { delete static_cast<VKShader*>(resource); }
    void destroy(PipelineState* resource) override  { delete static_cast<VKPipelineState*>(resource); }
    void destroy(CommandList* resource) override    { delete static_cast<VKCommandList*>(resource); }
    void destroy(SwapChain* resource) override {
        auto it = std::find(m_swapChains.begin(), m_swapChains.end(), resource);
        if (it != m_swapChains.end()) m_swapChains.erase(it);
        delete static_cast<VKSwapChain*>(resource);
    }
    void destroy(Fence* resource) override          { delete static_cast<VKFence*>(resource); }

    // --- 提交命令 ---

    void executeCommandLists(CommandList** cmdLists,
                             uint32_t count,
                             Fence* fence = nullptr,
                             uint64_t fenceValue = 0) override {
        std::vector<vk::CommandBuffer> cmdBuffers(count);
        for (uint32_t i = 0; i < count; ++i) {
            cmdBuffers[i] = static_cast<VKCommandList*>(cmdLists[i])->cmdBuffer();
        }

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = count;
        submitInfo.pCommandBuffers    = cmdBuffers.data();

        if (fence) {
            auto* vkFence = static_cast<VKFence*>(fence);
            vk::TimelineSemaphoreSubmitInfo timelineInfo;
            timelineInfo.signalSemaphoreValueCount = 1;
            uint64_t signalValue = fenceValue;
            timelineInfo.pSignalSemaphoreValues = &signalValue;

            vk::Semaphore semaphore = vkFence->semaphore();
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores    = &semaphore;
            submitInfo.pNext                = &timelineInfo;

            m_graphicsQueue.submit(submitInfo);
        } else {
            m_graphicsQueue.submit(submitInfo);
        }
    }

    void waitIdle() override {
        m_device.waitIdle();
    }

    // --------------------------------------------------------
    // 帧循环（高级接口）
    // --------------------------------------------------------

    /// 每帧开头：等待上一轮完成、重置命令缓冲区和 descriptor pool
    void beginFrame() {
        auto& frame = currentFrameContext();
        frame.waitForFence();
        frame.resetFence();
        frame.resetCommandBuffer();

        m_descriptorAllocator->resetPools();
    }

    /// 获取当前帧的主 command buffer
    vk::CommandBuffer frameCmdBuffer() {
        return currentFrameContext().cmdBuffer();
    }

    /// 提交当前帧 + present（完整渲染循环）
    void presentFrame(VKSwapChain* swapchain) {
        auto& frame = currentFrameContext();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        vk::CommandBuffer cmdBuf = frame.cmdBuffer();
        submitInfo.pCommandBuffers = &cmdBuf;

        vk::Semaphore waitSemaphores[] = { frame.imageAvailable() };
        vk::PipelineStageFlags waitStages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        };
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = waitStages;

        vk::Semaphore signalSemaphores[] = { frame.renderFinished() };
        submitInfo.signalSemaphoreCount  = 1;
        submitInfo.pSignalSemaphores     = signalSemaphores;

        m_graphicsQueue.submit(submitInfo, frame.inFlightFence());

        swapchain->presentWithSemaphores(frame.renderFinished());

        m_currentFrame = (m_currentFrame + 1) % m_frameCount;
    }

    // --- Vulkan 特有访问器 ---

    vk::Instance                vkInstance()          const { return m_instance; }
    vk::PhysicalDevice          vkPhysicalDevice()    const { return m_physicalDevice; }
    vk::Device                  vkDevice()            const { return m_device; }
    vk::Queue                   graphicsQueue()       const { return m_graphicsQueue; }
    uint32_t                    graphicsQueueFamily() const { return m_graphicsQueueFamily; }
    VmaAllocator               vmaAllocator()        const { return m_allocator; }

    VKUploadContext&            uploadContext()       { return *m_uploadContext; }
    VKDescriptorAllocator&      descriptorAllocator() { return *m_descriptorAllocator; }
    VKFrameContext&             currentFrameContext() { return *m_frameContexts[m_currentFrame]; }
    uint32_t                    currentFrameIndex()   const { return m_currentFrame; }
    const RenderConfig&         renderConfig()        const { return m_renderConfig; }

private:
    void init(const CreateInfo& ci);
    void shutdown();
    void pickPhysicalDevice(const std::vector<vk::PhysicalDevice>& devices);
    void createLogicalDevice(bool enableValidation);
    vk::SurfaceKHR createSurface(const NativeWindowHandle& window);

    void initFrameContexts(uint32_t count) {
        m_frameContexts.clear();
        m_frameCount = count;
        for (uint32_t i = 0; i < count; ++i) {
            m_frameContexts.push_back(
                std::make_unique<VKFrameContext>(m_device, m_graphicsQueueFamily));
        }
        m_currentFrame = 0;
    }

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

    uint32_t                    m_frameCount   = 2;
    uint32_t                    m_currentFrame = 0;
};

} // namespace MulanGeo::Engine
