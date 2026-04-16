/**
 * @file VKFrameContext.h
 * @brief Vulkan帧上下文，每帧独立的同步与命令资源
 * @author hxxcxx
 * @date 2026-04-16
 */

#pragma once

#include "VkCommon.h"

namespace MulanGeo::Engine {

class VKFrameContext {
public:
    VKFrameContext(vk::Device device, uint32_t queueFamily)
        : m_device(device)
    {
        // Command pool — 每帧可 reset
        vk::CommandPoolCreateInfo poolCI;
        poolCI.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolCI.queueFamilyIndex = queueFamily;
        m_cmdPool = m_device.createCommandPool(poolCI);

        // 主 command buffer
        vk::CommandBufferAllocateInfo allocCI;
        allocCI.commandPool        = m_cmdPool;
        allocCI.level              = vk::CommandBufferLevel::ePrimary;
        allocCI.commandBufferCount = 1;
        auto bufs = m_device.allocateCommandBuffers(allocCI);
        m_cmdBuffer = bufs[0];

        // 同步对象
        m_imageAvailable = m_device.createSemaphore({});
        m_renderFinished = m_device.createSemaphore({});

        vk::FenceCreateInfo fenceCI;
        fenceCI.flags = vk::FenceCreateFlagBits::eSignaled; // 初始为 signaled，第一帧不用等
        m_inFlightFence = m_device.createFence(fenceCI);
    }

    ~VKFrameContext() {
        if (m_inFlightFence) m_device.destroyFence(m_inFlightFence);
        if (m_renderFinished) m_device.destroySemaphore(m_renderFinished);
        if (m_imageAvailable) m_device.destroySemaphore(m_imageAvailable);
        if (m_cmdPool) {
            // command buffer 随 pool 销毁
            m_device.destroyCommandPool(m_cmdPool);
        }
    }

    // 禁止拷贝/移动
    VKFrameContext(const VKFrameContext&) = delete;
    VKFrameContext& operator=(const VKFrameContext&) = delete;

    // --------------------------------------------------------
    // 帧生命周期
    // --------------------------------------------------------

    /// 等待上一轮 GPU 工作完成（帧开头调用）
    void waitForFence() {
        m_device.waitForFences(m_inFlightFence, true, UINT64_MAX);
    }

    /// 重置 fence，准备本帧录制
    void resetFence() {
        m_device.resetFences(m_inFlightFence);
    }

    /// 重置 command buffer（pool 级别）
    void resetCommandBuffer() {
        m_device.resetCommandPool(m_cmdPool);
    }

    // --------------------------------------------------------
    // 访问器
    // --------------------------------------------------------

    vk::CommandBuffer  cmdBuffer()       const { return m_cmdBuffer; }
    vk::CommandPool    cmdPool()         const { return m_cmdPool; }
    vk::Semaphore      imageAvailable()  const { return m_imageAvailable; }
    vk::Semaphore      renderFinished()  const { return m_renderFinished; }
    vk::Fence          inFlightFence()   const { return m_inFlightFence; }

private:
    vk::Device         m_device;
    vk::CommandPool    m_cmdPool;
    vk::CommandBuffer  m_cmdBuffer;
    vk::Semaphore      m_imageAvailable;
    vk::Semaphore      m_renderFinished;
    vk::Fence          m_inFlightFence;
};

} // namespace MulanGeo::Engine
