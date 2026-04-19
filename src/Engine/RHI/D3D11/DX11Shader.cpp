/**
 * @file DX11Shader.cpp
 * @brief D3D11 着色器实现
 * @author zmb
 * @date 2026-04-19
 */
#include "DX11Shader.h"

namespace MulanGeo::Engine
{

DX11Shader::DX11Shader(const ShaderDesc& desc, ID3D11Device* device)
    : m_desc(desc)
{
    if (desc.byteCode && desc.byteCodeSize > 0)
    {
        m_byteCode.assign(desc.byteCode, desc.byteCode + desc.byteCodeSize);
    }

    if (m_byteCode.empty()) return;

    const void* code = m_byteCode.data();
    size_t size = m_byteCode.size();

    HRESULT hr = S_OK;
    switch (desc.type)
    {
    case ShaderType::Vertex:
        hr = device->CreateVertexShader(code, size, nullptr, &m_vs);
        DX11_CHECK(hr);
        break;
    case ShaderType::Pixel:
        hr = device->CreatePixelShader(code, size, nullptr, &m_ps);
        DX11_CHECK(hr);
        break;
    case ShaderType::Geometry:
        hr = device->CreateGeometryShader(code, size, nullptr, &m_gs);
        DX11_CHECK(hr);
        break;
    default:
        break;
    }
}

} // namespace MulanGeo::Engine
