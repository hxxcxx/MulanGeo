/*
 * Vulkan Descriptor 分配器 — Pool 管理与 Set 分配
 *
 * 负责：
 * - 管理 vk::DescriptorPool（按需增长，用尽时自动创建新 pool）
 * - 从 pool 中分配 descriptor set
 * - 每 frame 开始时 reset pool（所有 set 自动失效）
 *
 * 使用方式：
 *   1. beginFrame() — reset 所有 pool
 *   2. allocate(layout) — 分配 descriptor set
 *   3. write/set 绑定 — 填充 descriptor set
 *   4. 帧结束 — 不需要手动释放
 */

#pragma once

#include "VkCommon.h"

#include <vector>
#include <array>
#include <cassert>

namespace MulanGeo::Engine {

class VKDescriptorAllocator {
public:
    // 每个 pool 的容量配置
    struct PoolSizes {
        std::vector<vk::DescriptorPoolSize> sizes;
    };

    VKDescriptorAllocator(vk::Device device,
                          const PoolSizes& poolSizes = defaultPoolSizes())
        : m_device(device)
        , m_poolSizes(poolSizes)
    {}

    ~VKDescriptorAllocator() {
        for (auto pool : m_pools) {
            m_device.destroyDescriptorPool(pool);
        }
    }

    // --------------------------------------------------------
    // 帧生命周期
    // --------------------------------------------------------

    /// 每帧开始：reset 所有 pool，已分配的 set 全部失效
    void resetPools() {
        for (auto pool : m_pools) {
            m_device.resetDescriptorPool(pool);
        }
        m_activePool = nullptr;
    }

    // --------------------------------------------------------
    // Descriptor Set 分配
    // --------------------------------------------------------

    /// 从 pool 中分配一个 descriptor set
    vk::DescriptorSet allocate(vk::DescriptorSetLayout layout) {
        // 惰性获取可用 pool
        if (!m_activePool) {
            m_activePool = getOrCreatePool();
        }

        vk::DescriptorSetAllocateInfo allocCI;
        allocCI.descriptorPool     = m_activePool;
        allocCI.descriptorSetCount = 1;
        allocCI.pSetLayouts       = &layout;

        try {
            auto sets = m_device.allocateDescriptorSets(allocCI);
            return sets[0];
        } catch (vk::OutOfPoolMemoryError&) {
            // 当前 pool 不够了，创建新的
        } catch (vk::FragmentedPoolError&) {
            // pool 碎片化，创建新的
        }

        m_activePool = createPool();
        allocCI.descriptorPool = m_activePool;
        auto sets = m_device.allocateDescriptorSets(allocCI);
        return sets[0];
    }

    // --------------------------------------------------------
    // Descriptor Set 写入（便捷方法）
    // --------------------------------------------------------

    /// 绑定 Uniform Buffer 到指定 binding
    void bindUniformBuffer(vk::DescriptorSet set, uint32_t binding,
                           vk::Buffer buffer, vk::DeviceSize offset,
                           vk::DeviceSize range) {
        vk::DescriptorBufferInfo bufInfo;
        bufInfo.buffer = buffer;
        bufInfo.offset = offset;
        bufInfo.range  = range;

        vk::WriteDescriptorSet write;
        write.dstSet           = set;
        write.dstBinding       = binding;
        write.dstArrayElement  = 0;
        write.descriptorCount  = 1;
        write.descriptorType   = vk::DescriptorType::eUniformBuffer;
        write.pBufferInfo      = &bufInfo;

        m_device.updateDescriptorSets(1, &write, 0, nullptr);
    }

    /// 批量写入（一次调用多个 write）
    void updateDescriptorSets(uint32_t writeCount,
                              const vk::WriteDescriptorSet* writes) {
        m_device.updateDescriptorSets(writeCount, writes, 0, nullptr);
    }

private:
    // --------------------------------------------------------
    // Pool 管理
    // --------------------------------------------------------

    vk::DescriptorPool getOrCreatePool() {
        if (!m_pools.empty()) {
            return m_pools.back();
        }
        return createPool();
    }

    vk::DescriptorPool createPool() {
        // 每个 pool 可以分配 1000 个 set
        vk::DescriptorPoolCreateInfo poolCI;
        poolCI.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolCI.maxSets       = 1000;
        poolCI.poolSizeCount = static_cast<uint32_t>(m_poolSizes.sizes.size());
        poolCI.pPoolSizes    = m_poolSizes.sizes.data();

        auto pool = m_device.createDescriptorPool(poolCI);
        m_pools.push_back(pool);
        return pool;
    }

    /// 默认 pool 容量 — 覆盖 CAD viewer 常用类型
    static PoolSizes defaultPoolSizes() {
        return {{
            { vk::DescriptorType::eUniformBuffer,         64 },
            { vk::DescriptorType::eCombinedImageSampler,  32 },
            { vk::DescriptorType::eStorageBuffer,         16 },
            { vk::DescriptorType::eUniformBufferDynamic,   8 },
        }};
    }

    vk::Device                        m_device;
    PoolSizes                         m_poolSizes;
    std::vector<vk::DescriptorPool>   m_pools;
    vk::DescriptorPool                m_activePool = nullptr;
};

} // namespace MulanGeo::Engine
