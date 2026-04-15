/*
 * Vulkan Buffer 实现
 */

#pragma once

#include "../Buffer.h"
#include "VkConvert.h"

namespace MulanGeo::Engine {

class VKBuffer : public Buffer {
public:
    VKBuffer(const BufferDesc& desc, VmaAllocator allocator)
        : m_desc(desc), m_allocator(allocator)
    {
        VkBufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ci.size  = desc.size;
        ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                 | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                 | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                 | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo{};
        switch (desc.usage) {
            case BufferUsage::Immutable:
                allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                if (desc.initData) allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;
            case BufferUsage::Default:
                allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
                break;
            case BufferUsage::Dynamic:
                allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
                allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                               | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                break;
            case BufferUsage::Staging:
                allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
                allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                               | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
                ci.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                break;
        }

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocResult;

        VkResult res = vmaCreateBuffer(allocator, &ci, &allocInfo,
                                       &buffer, &allocation, &allocResult);
        if (res != VK_SUCCESS) return;

        m_buffer     = vk::Buffer(buffer);
        m_allocation = allocation;
        m_mappedData = allocResult.pMappedData;

        // 上传初始数据
        if (desc.initData && m_mappedData) {
            memcpy(m_mappedData, desc.initData, desc.size);
        } else if (desc.initData) {
            // 不可映射的 immutable buffer 需要暂存缓冲区（后续由 VKDevice 处理）
            // 暂时标记需要上传
            m_pendingData = desc.initData;
        }
    }

    ~VKBuffer() {
        if (m_buffer && m_allocator) {
            vmaDestroyBuffer(m_allocator, VkBuffer(m_buffer), m_allocation);
        }
    }

    const BufferDesc& desc() const override { return m_desc; }

    vk::Buffer vkBuffer() const { return m_buffer; }
    VmaAllocation allocation() const { return m_allocation; }
    void* mappedData() const { return m_mappedData; }
    const void* pendingData() const { return m_pendingData; }
    bool needsUpload() const { return m_pendingData != nullptr; }
    void markUploaded() { m_pendingData = nullptr; }

    // 动态更新
    void update(uint32_t offset, uint32_t size, const void* data) {
        if (m_mappedData) {
            memcpy(static_cast<uint8_t*>(m_mappedData) + offset, data, size);
        }
    }

private:
    BufferDesc      m_desc;
    VmaAllocator    m_allocator = nullptr;
    vk::Buffer      m_buffer;
    VmaAllocation   m_allocation = nullptr;
    void*           m_mappedData  = nullptr;
    const void*     m_pendingData = nullptr;
};

} // namespace MulanGeo::Engine
