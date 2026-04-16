/*
 * Vulkan 上传管理器 — Staging Buffer 池 + GPU 传输
 *
 * 负责：
 * - 管理 staging buffer 池，避免频繁 alloc/free
 * - 将 CPU 数据上传到 GPU-only 的 buffer/image
 * - 支持同步（fence 等待）和异步（外部 fence）两种模式
 */

#pragma once

#include "VkCommon.h"

#include <vector>
#include <mutex>
#include <cassert>

namespace MulanGeo::Engine {

// 前向声明
class VKBuffer;

/// 一次性的 staging slice，用完归还给池
struct StagingSlice {
    vk::Buffer  buffer     = {};
    VmaAllocation allocation = nullptr;
    void*       mapped     = nullptr;
    uint32_t    offset     = 0;
    uint32_t    size       = 0;
};

class VKUploadContext {
public:
    VKUploadContext(vk::Device device, VmaAllocator allocator,
                    uint32_t queueFamily, vk::Queue queue)
        : m_device(device)
        , m_allocator(allocator)
        , m_queueFamily(queueFamily)
        , m_queue(queue)
    {
        // 创建专用 command pool（transient，每批上传重置）
        vk::CommandPoolCreateInfo poolCI;
        poolCI.flags            = vk::CommandPoolCreateFlagBits::eTransient;
        poolCI.queueFamilyIndex = m_queueFamily;
        m_cmdPool = m_device.createCommandPool(poolCI);

        // 创建 fence 用于同步上传完成
        m_uploadFence = m_device.createFence({});
    }

    ~VKUploadContext() {
        flush(); // 确保所有 staging buffer 已回收

        // 销毁池中所有 staging buffer
        for (auto& slab : m_slabs) {
            vmaDestroyBuffer(m_allocator, VkBuffer(slab.buffer), slab.allocation);
        }
        m_slabs.clear();

        if (m_uploadFence) m_device.destroyFence(m_uploadFence);
        if (m_cmdPool)     m_device.destroyCommandPool(m_cmdPool);
    }

    // --------------------------------------------------------
    // Buffer 上传（同步）
    // --------------------------------------------------------

    /// 将 data 上传到目标 GPU buffer，阻塞直到 GPU 完成
    void uploadToBuffer(VKBuffer* dst, const void* data, uint32_t size,
                        uint32_t dstOffset = 0) {
        auto slice = allocStaging(size);
        memcpy(slice.mapped, data, size);

        // 刷新 mapped memory（非 coherent heap 需要）
        vmaFlushAllocation(m_allocator, slice.allocation, slice.offset, size);

        executeCopy(
            // copy lambda
            [&](vk::CommandBuffer cmd) {
                vk::BufferCopy region;
                region.srcOffset = slice.offset;
                region.dstOffset = dstOffset;
                region.size      = size;
                cmd.copyBuffer(slice.buffer, dst->vkBuffer(), 1, &region);
            }
        );
    }

    /// 一次性上传 buffer 的全部初始数据（VKBuffer::m_pendingData 路径）
    void uploadBufferInit(VKBuffer* dst) {
        assert(dst->pendingData() && dst->needsUpload());
        uploadToBuffer(dst, dst->pendingData(), dst->desc().size);
        dst->markUploaded();
    }

    // --------------------------------------------------------
    // Staging 池管理
    // --------------------------------------------------------

    /// 分配一块 staging 内存，用完后通过 flush() 批量归还
    StagingSlice allocStaging(uint32_t size) {
        std::lock_guard lock(m_mutex);

        // 从现有 slab 中分配（bump allocator）
        for (auto& slab : m_slabs) {
            if (slab.used + size <= slab.capacity) {
                StagingSlice slice;
                slice.buffer     = slab.buffer;
                slice.allocation = slab.allocation;
                slice.mapped     = slab.mapped;
                slice.offset     = slab.used;
                slice.size       = size;
                slab.used       += size;
                return slice;
            }
        }

        // 没有可用 slab，创建新的（至少 4MB，向上取整到 size）
        uint32_t slabSize = (std::max)(uint32_t(4 * 1024 * 1024), alignUp(size, 256));
        auto slab = createSlab(slabSize);
        m_slabs.push_back(slab);

        StagingSlice slice;
        slice.buffer     = slab.buffer;
        slice.allocation = slab.allocation;
        slice.mapped     = slab.mapped;
        slice.offset     = 0;
        slice.size       = size;
        slab.used        = size;
        return slice;
    }

    /// 重置所有 slab（在 fence wait 之后调用）
    void resetSlabs() {
        std::lock_guard lock(m_mutex);
        for (auto& slab : m_slabs) {
            slab.used = 0;
        }
    }

    /// 等待 GPU 完成并重置 slab 池
    void flush() {
        if (m_pending) {
            m_device.waitForFences(m_uploadFence, true, UINT64_MAX);
            m_device.resetFences(m_uploadFence);
            m_pending = false;
            resetSlabs();
        }
    }

private:
    // --------------------------------------------------------
    // 通用 copy 执行器
    // --------------------------------------------------------

    template <typename F>
    void executeCopy(F&& copyCmd) {
        // 分配 command buffer
        vk::CommandBufferAllocateInfo allocCI;
        allocCI.commandPool        = m_cmdPool;
        allocCI.level              = vk::CommandBufferLevel::ePrimary;
        allocCI.commandBufferCount = 1;
        auto cmds = m_device.allocateCommandBuffers(allocCI);

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        cmds[0].begin(beginInfo);

        // 执行 copy 命令
        copyCmd(cmds[0]);

        cmds[0].end();

        // 提交并等待
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &cmds[0];
        m_queue.submit(submitInfo, m_uploadFence);

        m_device.waitForFences(m_uploadFence, true, UINT64_MAX);
        m_device.resetFences(m_uploadFence);

        // 回收 command buffer + 重置 pool
        m_device.freeCommandBuffers(m_cmdPool, cmds);
        m_device.resetCommandPool(m_cmdPool);
        resetSlabs();
    }

    // --------------------------------------------------------
    // Slab 管理
    // --------------------------------------------------------

    struct Slab {
        vk::Buffer      buffer;
        VmaAllocation   allocation = nullptr;
        void*           mapped     = nullptr;
        uint32_t        capacity   = 0;
        uint32_t        used       = 0;
    };

    Slab createSlab(uint32_t size) {
        VkBufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ci.size  = size;
        ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                      | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
        vmaCreateBuffer(m_allocator, &ci, &allocCI, &buffer, &allocation, &info);

        Slab slab;
        slab.buffer     = vk::Buffer(buffer);
        slab.allocation = allocation;
        slab.mapped     = info.pMappedData;
        slab.capacity   = size;
        slab.used       = 0;
        return slab;
    }

    static uint32_t alignUp(uint32_t v, uint32_t align) {
        return (v + align - 1) & ~(align - 1);
    }

    vk::Device       m_device;
    VmaAllocator     m_allocator;
    uint32_t         m_queueFamily;
    vk::Queue        m_queue;

    vk::CommandPool  m_cmdPool;
    vk::Fence        m_uploadFence;
    bool             m_pending = false;

    std::vector<Slab> m_slabs;
    std::mutex        m_mutex;
};

} // namespace MulanGeo::Engine
