/*
 * Vertex Buffer — runtime vertex data access and construction
 *
 * Provides VertexElement (typed view), VertexBufferView (read-only
 * accessor), VertexBufferBuilder (mutable builder), and color packing.
 */

#pragma once

#include "VertexLayout.h"

#include <cstring>
#include <vector>
#include <cstdint>

namespace MulanGeo::Engine {

// ============================================================
// Color packing utilities
// ============================================================

// Pack RGBA [0-255] into a single uint32_t
constexpr uint32_t packColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return static_cast<uint32_t>(r)
         | (static_cast<uint32_t>(g) << 8)
         | (static_cast<uint32_t>(b) << 16)
         | (static_cast<uint32_t>(a) << 24);
}

// Pack RGBA [0.0-1.0] into uint32_t
constexpr uint32_t packColorF(float r, float g, float b, float a = 1.0f) {
    return packColor(
        static_cast<uint8_t>(r * 255.0f),
        static_cast<uint8_t>(g * 255.0f),
        static_cast<uint8_t>(b * 255.0f),
        static_cast<uint8_t>(a * 255.0f)
    );
}

// ============================================================
// Vertex Element (typed view into a single attribute)
// ============================================================

template<VertexFormat F>
struct VertexElement {
    static constexpr VertexFormat format = F;
    static constexpr uint8_t byteSize = vertexFormatSize(F);

    // Storage: raw bytes, properly aligned
    alignas(4) std::byte data_[byteSize];

    using value_type = typename VertexFormatTraits<F>::type;

    constexpr value_type get() const {
        value_type result;
        std::memcpy(&result, data_, sizeof(result));
        return result;
    }

    constexpr void set(const value_type& v) {
        std::memcpy(data_, &v, sizeof(v));
    }
};

// ============================================================
// Vertex Buffer View (read-only accessor)
// ============================================================

class VertexBufferView {
public:
    constexpr VertexBufferView(std::span<const std::byte> data,
                               const VertexLayout& layout)
        : m_data(data), m_layout(layout) {}

    constexpr uint32_t vertexCount() const {
        return m_layout.stride() > 0
            ? static_cast<uint32_t>(m_data.size()) / m_layout.stride()
            : 0;
    }

    // Get raw pointer to attribute for vertex index
    constexpr const std::byte* attributeData(uint32_t vertexIndex,
                                              VertexSemantic sem) const {
        auto* attr = m_layout.find(sem);
        if (!attr) return nullptr;
        auto offset = vertexIndex * m_layout.stride() + attr->offset;
        if (offset + attr->size() > m_data.size()) return nullptr;
        return m_data.data() + offset;
    }

    // Typed read — returns value by copy
    template<typename T>
    constexpr T read(uint32_t vertexIndex, VertexSemantic sem) const {
        auto* ptr = attributeData(vertexIndex, sem);
        if (!ptr) return T{};
        T result;
        std::memcpy(&result, ptr, sizeof(T));
        return result;
    }

    // Convenience: read float3 position
    struct Float3 { float x, y, z; };
    Float3 readPosition(uint32_t idx) const {
        return read<Float3>(idx, VertexSemantic::Position);
    }

    // Convenience: read packed color
    uint32_t readColor(uint32_t idx) const {
        return read<uint32_t>(idx, VertexSemantic::Color0);
    }

private:
    std::span<const std::byte> m_data;
    VertexLayout m_layout;
};

// ============================================================
// Vertex Buffer Builder (mutable builder)
// ============================================================

class VertexBufferBuilder {
public:
    VertexBufferBuilder(const VertexLayout& layout, uint32_t maxVertices)
        : m_layout(layout), m_maxVertices(maxVertices)
    {
        m_buffer.resize(static_cast<size_t>(maxVertices) * layout.stride());
    }

    uint32_t stride() const { return m_layout.stride(); }
    uint32_t capacity() const { return m_maxVertices; }

    // Typed write
    template<typename T>
    void write(uint32_t vertexIndex, VertexSemantic sem, const T& value) {
        auto* attr = m_layout.find(sem);
        if (!attr) return;
        auto offset = vertexIndex * m_layout.stride() + attr->offset;
        std::memcpy(m_buffer.data() + offset, &value, sizeof(T));
    }

    void setPosition(uint32_t idx, float x, float y, float z) {
        float pos[3] = {x, y, z};
        write(idx, VertexSemantic::Position, pos);
    }

    void setNormal(uint32_t idx, float x, float y, float z) {
        float n[3] = {x, y, z};
        write(idx, VertexSemantic::Normal, n);
    }

    void setColor(uint32_t idx, uint32_t packedRGBA) {
        write(idx, VertexSemantic::Color0, packedRGBA);
    }

    void setColor(uint32_t idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        setColor(idx, packColor(r, g, b, a));
    }

    void setPickId(uint32_t idx, uint32_t id) {
        write(idx, VertexSemantic::PickId, id);
    }

    // Get the built data
    std::span<const std::byte> data() const {
        return {reinterpret_cast<const std::byte*>(m_buffer.data()), m_buffer.size()};
    }

    std::span<std::byte> mutableData() {
        return {reinterpret_cast<std::byte*>(m_buffer.data()), m_buffer.size()};
    }

    size_t sizeBytes() const { return m_buffer.size(); }
    const VertexLayout& layout() const { return m_layout; }

private:
    VertexLayout m_layout;
    uint32_t m_maxVertices;
    std::vector<std::byte> m_buffer;
};

} // namespace MulanGeo::Engine
