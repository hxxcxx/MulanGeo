/**
 * @file VKBuffer.h
 * @brief Vulkan缓冲区实现
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../Buffer.h"
#include "VkConvert.h"

namespace MulanGeo::Engine {

class VKBuffer : public Buffer {
public:
    VKBuffer(const BufferDesc& desc, VmaAllocator allocator);
    ~VKBuffer();

    const BufferDesc& desc() const override { return m_desc; }

    vk::Buffer vkBuffer() const { return m_buffer; }
    VmaAllocation allocation() const { return m_allocation; }
    void* mappedData() const { return m_mappedData; }
    const void* pendingData() const { return m_pendingData; }
    bool needsUpload() const { return m_pendingData != nullptr; }
    void markUploaded() { m_pendingData = nullptr; }

    void update(uint32_t offset, uint32_t size, const void* data) override;
    bool readback(uint32_t offset, uint32_t size, void* outData) override;

private:
    BufferDesc      m_desc;
    VmaAllocator    m_allocator = nullptr;
    vk::Buffer      m_buffer;
    VmaAllocation   m_allocation = nullptr;
    void*           m_mappedData  = nullptr;
    const void*     m_pendingData = nullptr;
};

} // namespace MulanGeo::Engine
