/**
 * @file DX12Texture.h
 * @brief D3D12 纹理实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#pragma once

#include "../Texture.h"
#include "DX12Common.h"

namespace MulanGeo::Engine {

class DX12Texture final : public Texture {
public:
    DX12Texture(const TextureDesc& desc, ID3D12Device* device,
                D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);
    ~DX12Texture();

    const TextureDesc& desc() const override { return m_desc; }

    ID3D12Resource* resource() const { return m_resource.Get(); }
    D3D12_RESOURCE_STATES state() const { return m_state; }
    void setState(D3D12_RESOURCE_STATES s) { m_state = s; }

private:
    TextureDesc               m_desc;
    ComPtr<ID3D12Resource>    m_resource;
    D3D12_RESOURCE_STATES     m_state;
};

} // namespace MulanGeo::Engine
