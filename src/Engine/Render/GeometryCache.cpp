#include "GeometryCache.h"

namespace MulanGeo::Engine {

GeometryCache::GeometryCache(RHIDevice* device)
    : m_device(device) {}

GeometryCache::~GeometryCache() { clear(); }

const GpuGeometryBuffers* GeometryCache::getOrUpload(const RenderGeometry* geo) {
    auto it = m_cache.find(geo);
    if (it != m_cache.end()) return &it->second;

    return upload(geo);
}

void GeometryCache::clear() {
    for (auto& [_, buf] : m_cache) {
        if (buf.vertexBuffer) m_device->destroy(buf.vertexBuffer);
        if (buf.indexBuffer)  m_device->destroy(buf.indexBuffer);
    }
    m_cache.clear();
}

const GpuGeometryBuffers* GeometryCache::upload(const RenderGeometry* geo) {
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

} // namespace MulanGeo::Engine
