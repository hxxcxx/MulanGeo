/**
 * @file GeometryCache.h
 * @brief 几何数据GPU缓存，避免重复上传顶点与索引缓冲区
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../RHI/Buffer.h"
#include "../RHI/Device.h"
#include "RenderGeometry.h"

#include <cstddef>
#include <unordered_map>

namespace MulanGeo::Engine {

// 单份几何的 GPU 资源
struct GpuGeometryBuffers {
    Buffer* vertexBuffer = nullptr;
    Buffer* indexBuffer  = nullptr;
    uint32_t vertexCount = 0;
    uint32_t indexCount  = 0;
    uint32_t vertexStride = 0;
};

class GeometryCache {
public:
    explicit GeometryCache(RHIDevice* device)
        : m_device(device) {}

    ~GeometryCache() { clear(); }

    // 获取或上传几何数据的 GPU 缓冲区
    // 以 RenderGeometry 指针为 key（调用方保证对象地址稳定）
    const GpuGeometryBuffers* getOrUpload(const RenderGeometry* geo) {
        auto it = m_cache.find(geo);
        if (it != m_cache.end()) return &it->second;

        return upload(geo);
    }

    // 释放所有 GPU 缓冲区
    void clear() {
        for (auto& [_, buf] : m_cache) {
            if (buf.vertexBuffer) m_device->destroy(buf.vertexBuffer);
            if (buf.indexBuffer)  m_device->destroy(buf.indexBuffer);
        }
        m_cache.clear();
    }

    size_t count() const { return m_cache.size(); }

private:
    const GpuGeometryBuffers* upload(const RenderGeometry* geo) {
        GpuGeometryBuffers buf;
        buf.vertexStride = geo->vertexStride;
        buf.vertexCount  = geo->vertexCount;
        buf.indexCount   = geo->indexCount;

        // 顶点缓冲区
        if (geo->vertexCount > 0 && !geo->vertexBytes.empty()) {
            auto desc = BufferDesc::vertex(
                static_cast<uint32_t>(geo->vertexBytes.size()),
                geo->vertexBytes.data(),
                "GeoVB");
            buf.vertexBuffer = m_device->createBuffer(desc);
        }

        // 索引缓冲区
        if (geo->indexCount > 0 && !geo->indexBytes.empty()) {
            auto desc = BufferDesc::index(
                static_cast<uint32_t>(geo->indexBytes.size()),
                geo->indexBytes.data(),
                "GeoIB");
            buf.indexBuffer = m_device->createBuffer(desc);
        }

        auto [it, _] = m_cache.emplace(geo, std::move(buf));
        return &it->second;
    }

    RHIDevice* m_device;
    std::unordered_map<const RenderGeometry*, GpuGeometryBuffers> m_cache;
};

} // namespace MulanGeo::Engine
