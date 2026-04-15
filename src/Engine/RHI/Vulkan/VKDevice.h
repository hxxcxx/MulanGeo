/*
 * Vulkan Device 实现 — 资源工厂与后端入口
 *
 * 管理 VkInstance、PhysicalDevice、Device、VMA Allocator。
 * 所有 GPU 资源的创建/销毁入口。
 */

#pragma once

#include "VkCommon.h"
#include "../Device.h"
#include "VkConvert.h"
#include "VKBuffer.h"
#include "VKTexture.h"
#include "VKShader.h"
#include "VKPipelineState.h"
#include "VKCommandList.h"
#include "VKSwapChain.h"
#include "VKFence.h"

#include <vector>
#include <set>
#include <cstdio>

namespace MulanGeo::Engine {

class VKDevice : public RHIDevice {
public:
    struct CreateInfo {
        bool enableValidation = true;
        void* windowHandle   = nullptr;   // HWND on Windows
        const char* appName  = "MulanGeo";
    };

    explicit VKDevice(const CreateInfo& ci = {}) {
        init(ci);
    }

    ~VKDevice() {
        m_device.waitIdle();
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
        // 如果有初始数据但 buffer 不可映射，需要 upload
        if (buf->needsUpload()) {
            uploadBufferData(buf);
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
        auto* pso = new VKPipelineState(desc, m_device);
        // 延迟 build — 需要 renderPass
        return pso;
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
        params.windowHandle        = m_windowHandle;

        auto* swapchain = new VKSwapChain(desc, params);
        m_swapChains.push_back(swapchain);
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
            // Timeline semaphore signal
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

    // --- Vulkan 特有访问器 ---

    vk::Instance       vkInstance()       const { return m_instance; }
    vk::PhysicalDevice vkPhysicalDevice() const { return m_physicalDevice; }
    vk::Device         vkDevice()         const { return m_device; }
    vk::Queue          graphicsQueue()    const { return m_graphicsQueue; }
    uint32_t           graphicsQueueFamily() const { return m_graphicsQueueFamily; }

private:
    void init(const CreateInfo& ci);
    void shutdown();
    void pickPhysicalDevice(const std::vector<vk::PhysicalDevice>& devices);
    void createLogicalDevice(bool enableValidation);
    void uploadBufferData(VKBuffer* buffer);

    vk::Instance                m_instance;
    vk::PhysicalDevice          m_physicalDevice;
    vk::Device                  m_device;
    vk::SurfaceKHR              m_surface;
    vk::Queue                   m_graphicsQueue;
    vk::Queue                   m_presentQueue;
    VmaAllocator                m_allocator = nullptr;

    uint32_t                    m_graphicsQueueFamily = 0;
    uint32_t                    m_presentQueueFamily  = 0;
    void*                       m_windowHandle = nullptr;

    DeviceCapabilities          m_caps;
    std::vector<VKSwapChain*>   m_swapChains;
};

} // namespace MulanGeo::Engine
