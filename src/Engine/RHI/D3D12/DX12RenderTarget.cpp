/**
 * @file DX12RenderTarget.cpp
 * @brief D3D12 离屏渲染目标实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#include "DX12RenderTarget.h"
#include "DX12CommandList.h"
#include "DX12Convert.h"

namespace MulanGeo::Engine {

DX12RenderTarget::DX12RenderTarget(const RenderTargetDesc& desc,
                                   ID3D12Device* device)
    : m_desc(desc)
    , m_device(device)
{
    createResources();
}

DX12RenderTarget::~DX12RenderTarget() = default;

void DX12RenderTarget::createResources() {
    // Color texture
    auto colorDesc = TextureDesc::renderTarget(m_desc.width, m_desc.height, m_desc.colorFormat);
    m_colorTexture = std::make_unique<DX12Texture>(
        colorDesc, m_device, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // RTV heap
    m_rtvHeap = std::make_unique<DX12DescriptorAllocator>(
        m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
    auto rtvDesc = m_rtvHeap->allocate();
    m_device->CreateRenderTargetView(
        m_colorTexture->resource(), nullptr, rtvDesc.cpu);
    m_rtvHandle = rtvDesc.cpu;

    if (m_desc.hasDepth) {
        // Depth texture
        auto depthDesc = TextureDesc::depthStencil(m_desc.width, m_desc.height, m_desc.depthFormat);
        m_depthTexture = std::make_unique<DX12Texture>(
            depthDesc, m_device, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // DSV heap
        m_dsvHeap = std::make_unique<DX12DescriptorAllocator>(
            m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
        auto dsvDesc = m_dsvHeap->allocate();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
        dsvViewDesc.Format        = toDSVFormat(m_desc.depthFormat);
        dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        m_device->CreateDepthStencilView(
            m_depthTexture->resource(), &dsvViewDesc, dsvDesc.cpu);
        m_dsvHandle = dsvDesc.cpu;
    }
}

void DX12RenderTarget::resize(uint32_t width, uint32_t height) {
    m_desc.width  = width;
    m_desc.height = height;
    m_colorTexture.reset();
    m_depthTexture.reset();
    m_rtvHeap.reset();
    m_dsvHeap.reset();
    createResources();
}

void DX12RenderTarget::beginRenderPass(CommandList* cmd) {
    auto* dx12Cmd = static_cast<DX12CommandList*>(cmd);
    auto* cl = dx12Cmd->commandList();

    // Transition color to RT
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_colorTexture->resource();
        barrier.Transition.StateBefore = m_colorTexture->state();
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        cl->ResourceBarrier(1, &barrier);
        m_colorTexture->setState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    cl->ClearRenderTargetView(m_rtvHandle, m_desc.clearColor, 0, nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = nullptr;
    if (m_depthTexture) {
        // Transition depth to DS write
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_depthTexture->resource();
        barrier.Transition.StateBefore = m_depthTexture->state();
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        cl->ResourceBarrier(1, &barrier);
        m_depthTexture->setState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

        cl->ClearDepthStencilView(m_dsvHandle,
                                   D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                   m_desc.clearDepth, 0, 0, nullptr);
        pDSV = &m_dsvHandle;
    }

    cl->OMSetRenderTargets(1, &m_rtvHandle, FALSE, pDSV);
}

void DX12RenderTarget::endRenderPass(CommandList* cmd) {
    auto* dx12Cmd = static_cast<DX12CommandList*>(cmd);
    auto* cl = dx12Cmd->commandList();

    // Transition back to shader resource
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_colorTexture->resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        cl->ResourceBarrier(1, &barrier);
        m_colorTexture->setState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

} // namespace MulanGeo::Engine
