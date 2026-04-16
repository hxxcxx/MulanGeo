/**
 * @file VKFence.h
 * @brief Vulkan栅栏实现
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../Fence.h"
#include "VkConvert.h"

namespace MulanGeo::Engine {

class VKFence : public Fence {
public:
    VKFence(vk::Device device, uint64_t initialValue)
        : m_device(device)
    {
        // 使用 timeline semaphore 实现 fence
        vk::SemaphoreTypeCreateInfo timelineCI;
        timelineCI.semaphoreType = vk::SemaphoreType::eTimeline;
        timelineCI.initialValue  = initialValue;

        vk::SemaphoreCreateInfo ci;
        ci.pNext = &timelineCI;

        m_semaphore = m_device.createSemaphore(ci);
    }

    ~VKFence() {
        if (m_semaphore) {
            m_device.destroySemaphore(m_semaphore);
        }
    }

    void signal(uint64_t value) override {
        vk::SemaphoreSignalInfo info;
        info.semaphore = m_semaphore;
        info.value     = value;
        m_device.signalSemaphore(info);
    }

    void wait(uint64_t value) override {
        vk::SemaphoreWaitInfo info;
        info.semaphoreCount = 1;
        vk::Semaphore semaphores[] = { m_semaphore };
        uint64_t values[] = { value };
        info.pSemaphores = semaphores;
        info.pValues     = values;
        m_device.waitSemaphores(info, UINT64_MAX);
    }

    uint64_t completedValue() const override {
        return m_device.getSemaphoreCounterValue(m_semaphore);
    }

    vk::Semaphore semaphore() const { return m_semaphore; }

private:
    vk::Device    m_device;
    vk::Semaphore m_semaphore;
};

} // namespace MulanGeo::Engine
