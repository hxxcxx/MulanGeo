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
    explicit GeometryCache(RHIDevice* device);
    ~GeometryCache();

    // 获取或上传几何数据的 GPU 缓冲区
    const GpuGeometryBuffers* getOrUpload(const RenderGeometry* geo);

    // 释放所有 GPU 缓冲区
    void clear();

    size_t count() const { return m_cache.size(); }

private:
    const GpuGeometryBuffers* upload(const RenderGeometry* geo);

    RHIDevice* m_device;
    // 用顶点数据指针做键（来自 IO 层，跨帧稳定）
    std::unordered_map<const void*, GpuGeometryBuffers> m_cache;
};

} // namespace MulanGeo::Engine
