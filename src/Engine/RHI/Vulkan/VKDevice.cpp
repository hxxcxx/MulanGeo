// VMA 实现（必须在 #include <vk_mem_alloc.h> 之前）
#define VMA_IMPLEMENTATION

#include "VKDevice.h"

// Vulkan-Hpp 动态 dispatch 存储（必须在 #include <vulkan/vulkan.hpp> 之后）
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <algorithm>

namespace MulanGeo::Engine {

// ============================================================
// 资源创建
// ============================================================

ResourcePtr<Buffer> VKDevice::createBuffer(const BufferDesc& desc) {
    auto* buf = new VKBuffer(desc, m_allocator);
    if (buf->needsUpload()) {
        m_uploadContext->uploadBufferInit(buf);
    }
    return ResourcePtr<Buffer>(buf, DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<Texture> VKDevice::createTexture(const TextureDesc& desc) {
    return ResourcePtr<Texture>(new VKTexture(desc, m_device, m_allocator), DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<Shader> VKDevice::createShader(const ShaderDesc& desc) {
    return ResourcePtr<Shader>(new VKShader(desc, m_device), DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<PipelineState> VKDevice::createPipelineState(const GraphicsPipelineDesc& desc) {
    return ResourcePtr<PipelineState>(new VKPipelineState(desc, m_device), DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<CommandList> VKDevice::createCommandList() {
    return ResourcePtr<CommandList>(new VKCommandList(m_device, m_graphicsQueueFamily), DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<SwapChain> VKDevice::createSwapChain(const SwapChainDesc& desc) {
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

    // 为每个 swapchain image 创建独立的 renderFinished 信号量
    for (uint32_t i = static_cast<uint32_t>(m_renderFinishedSemaphores.size());
         i < swapchain->imageCount(); ++i) {
        m_renderFinishedSemaphores.push_back(m_device.createSemaphore({}));
    }

    if (m_frameContexts.empty()) {
        initFrameContexts(m_frameCount);
    }

    return ResourcePtr<SwapChain>(swapchain, DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<Fence> VKDevice::createFence(uint64_t initialValue) {
    return ResourcePtr<Fence>(new VKFence(m_device, initialValue), DeviceResourceDeleter{shared_from_this()});
}

ResourcePtr<RenderTarget> VKDevice::createRenderTarget(const RenderTargetDesc& desc) {
    auto* rt = new VKRenderTarget(desc, m_device, m_allocator);

    // 如果 frame contexts 还未初始化（无 SwapChain 时），在此初始化
    if (m_frameContexts.empty()) {
        initFrameContexts(m_frameCount);
    }

    return ResourcePtr<RenderTarget>(rt, DeviceResourceDeleter{shared_from_this()});
}

// ============================================================
// 资源销毁
// ============================================================

void VKDevice::destroy(Buffer* resource)         { delete static_cast<VKBuffer*>(resource); }
void VKDevice::destroy(Texture* resource)        { delete static_cast<VKTexture*>(resource); }
void VKDevice::destroy(Shader* resource)         { delete static_cast<VKShader*>(resource); }
void VKDevice::destroy(PipelineState* resource)  { delete static_cast<VKPipelineState*>(resource); }
void VKDevice::destroy(CommandList* resource)    { delete static_cast<VKCommandList*>(resource); }
void VKDevice::destroy(Fence* resource)          { delete static_cast<VKFence*>(resource); }
void VKDevice::destroy(RenderTarget* resource)   { delete static_cast<VKRenderTarget*>(resource); }

void VKDevice::destroy(SwapChain* resource) {
    auto it = std::find(m_swapChains.begin(), m_swapChains.end(), resource);
    if (it != m_swapChains.end()) m_swapChains.erase(it);
    delete static_cast<VKSwapChain*>(resource);
}

// ============================================================
// 提交命令
// ============================================================

void VKDevice::executeCommandLists(CommandList** cmdLists, uint32_t count,
                                   Fence* fence, uint64_t fenceValue) {
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

void VKDevice::waitIdle() {
    m_device.waitIdle();
}

// ============================================================
// 帧循环
// ============================================================

void VKDevice::beginFrame() {
    auto& frame = currentFrameContext();
    frame.waitForFence();
    frame.resetFence();

    frame.resetCommandBuffer();

    m_descriptorAllocators[m_currentFrame]->resetPools();

    m_frameCmdList = std::make_unique<VKCommandList>(frame.cmdBuffer());

    if (!m_swapChains.empty()) {
        auto* sc = m_swapChains[0];
        sc->acquireNextImage(frame.imageAvailable());
        m_acquiredImageIndex = sc->currentImageIndex();
    }
}

CommandList* VKDevice::frameCommandList() {
    return m_frameCmdList.get();
}

void VKDevice::submitAndPresent(SwapChain* swapchain) {
    auto* vkSC  = static_cast<VKSwapChain*>(swapchain);
    auto& frame = currentFrameContext();

    // 使用 per-image 的 renderFinished 信号量
    vk::Semaphore renderFinished = m_renderFinishedSemaphores[m_acquiredImageIndex];

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    vk::CommandBuffer cmdBuf = static_cast<VKCommandList*>(m_frameCmdList.get())->cmdBuffer();
    submitInfo.pCommandBuffers = &cmdBuf;

    vk::Semaphore waitSemaphores[] = { frame.imageAvailable() };
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;

    submitInfo.signalSemaphoreCount  = 1;
    submitInfo.pSignalSemaphores     = &renderFinished;

    m_graphicsQueue.submit(submitInfo, frame.inFlightFence());

    vkSC->presentWithSemaphores(renderFinished);

    m_currentFrame = (m_currentFrame + 1) % m_frameCount;
}

void VKDevice::submitOffscreen() {
    auto& frame = currentFrameContext();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    vk::CommandBuffer cmdBuf = static_cast<VKCommandList*>(m_frameCmdList.get())->cmdBuffer();
    submitInfo.pCommandBuffers = &cmdBuf;

    m_graphicsQueue.submit(submitInfo, frame.inFlightFence());

    m_currentFrame = (m_currentFrame + 1) % m_frameCount;
}

// ============================================================
// Descriptor 绑定
// ============================================================

void VKDevice::bindUniformBuffers(CommandList* cmd, PipelineState* pso,
                                  const UniformBufferBind* binds, uint32_t count) {
    auto* vkPso  = static_cast<VKPipelineState*>(pso);
    auto* vkCmd  = static_cast<VKCommandList*>(cmd);

    vk::DescriptorSet descSet = m_descriptorAllocators[m_currentFrame]->allocate(
        vkPso->descriptorSetLayout());

    for (uint32_t i = 0; i < count; ++i) {
        auto* vkBuf = static_cast<VKBuffer*>(binds[i].buffer);
        m_descriptorAllocators[m_currentFrame]->bindUniformBuffer(
            descSet, binds[i].binding,
            vkBuf->vkBuffer(), binds[i].offset, binds[i].size);
    }

    vkCmd->bindDescriptorSet(vkPso->layout(), descSet);
}

// ============================================================
// 帧上下文初始化
// ============================================================

void VKDevice::initFrameContexts(uint32_t count) {
    m_frameContexts.clear();
    m_frameCount = count;
    for (uint32_t i = 0; i < count; ++i) {
        m_frameContexts.push_back(
            std::make_unique<VKFrameContext>(m_device, m_graphicsQueueFamily));
    }

    // 同步 per-frame descriptor allocator 数量
    m_descriptorAllocators.clear();
    for (uint32_t i = 0; i < count; ++i) {
        m_descriptorAllocators.push_back(
            std::make_unique<VKDescriptorAllocator>(m_device));
    }

    m_currentFrame = 0;
    m_frameCmdList = std::make_unique<VKCommandList>(
        currentFrameContext().cmdBuffer());
}

} // namespace MulanGeo::Engine
