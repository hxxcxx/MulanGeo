/**
 * @file DX12SwapChain.cpp
 * @brief D3D12 交换链实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#include "DX12SwapChain.h"
#include "DX12CommandList.h"
#include "DX12Convert.h"

namespace MulanGeo::Engine {

DX12SwapChain::DX12SwapChain(const SwapChainDesc& desc, ID3D12Device* device,
                             IDXGIFactory4* factory, ID3D12CommandQueue* queue,
                             const NativeWindowHandle& window)
    : m_desc(desc)
    , m_device(device)
    , m_queue(queue)
{
    m_clearColor[0] = 0.15f; m_clearColor[1] = 0.15f;
    m_clearColor[2] = 0.15f; m_clearColor[3] = 1.0f;

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width              = desc.width;
    scDesc.Height             = desc.height;
    scDesc.Format             = toDXGIFormat(desc.format);
    scDesc.Stereo             = FALSE;
    scDesc.SampleDesc.Count   = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount        = desc.bufferCount;
    scDesc.Scaling            = DXGI_SCALING_STRETCH;
    scDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    scDesc.Flags              = desc.vsync ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    HWND hwnd = reinterpret_cast<HWND>(window.win32.hWnd);
    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = factory->CreateSwapChainForHwnd(
        queue, hwnd, &scDesc, nullptr, nullptr, &swapChain1);
    DX12_CHECK(hr);

    hr = swapChain1.As(&m_swapChain);
    DX12_CHECK(hr);

    createRTVHeap();
    createBackBuffers();
}

DX12SwapChain::~DX12SwapChain() {
    releaseBackBuffers();
}

void DX12SwapChain::createRTVHeap() {
    m_rtvHeap = std::make_unique<DX12DescriptorAllocator>(
        m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE, m_desc.bufferCount);

    m_dsvHeap = std::make_unique<DX12DescriptorAllocator>(
        m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
}

void DX12SwapChain::createBackBuffers() {
    m_backBuffers.resize(m_desc.bufferCount);
    m_backBufferTextures.resize(m_desc.bufferCount);

    for (uint32_t i = 0; i < m_desc.bufferCount; ++i) {
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        DX12_CHECK(hr);

        auto rtvDesc = m_rtvHeap->allocate();
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvDesc.cpu);

        // 创建 DX12Texture 包装
        TextureDesc texDesc = TextureDesc::renderTarget(m_desc.width, m_desc.height, m_desc.format);
        m_backBufferTextures[i] = std::make_unique<DX12Texture>(texDesc, m_device);
        // 替换内部 resource 为 swap chain buffer
        // DX12Texture 内部已有 resource，但我们需要用 swap chain 的
        // 这里简化：直接在 render pass 中使用 m_backBuffers
    }

    // Depth stencil
    D3D12_RESOURCE_DESC dsDesc = {};
    dsDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsDesc.Alignment          = 0;
    dsDesc.Width              = m_desc.width;
    dsDesc.Height             = m_desc.height;
    dsDesc.DepthOrArraySize   = 1;
    dsDesc.MipLevels          = 1;
    dsDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsDesc.SampleDesc.Count   = 1;
    dsDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearVal.DepthStencil.Depth = 1.0f;

    ComPtr<ID3D12Resource> depthResource;
    m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearVal, IID_PPV_ARGS(&depthResource));

    auto dsvDesc = m_dsvHeap->allocate();
    m_device->CreateDepthStencilView(depthResource.Get(), nullptr, dsvDesc.cpu);

    m_depthTexture = std::make_unique<DX12Texture>(
        TextureDesc::depthStencil(m_desc.width, m_desc.height),
        m_device, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void DX12SwapChain::releaseBackBuffers() {
    m_backBufferTextures.clear();
    m_backBuffers.clear();
    m_depthTexture.reset();
    m_rtvHeap.reset();
    m_dsvHeap.reset();
}

Texture* DX12SwapChain::currentBackBuffer() {
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    if (m_frameIndex < m_backBufferTextures.size()) {
        return m_backBufferTextures[m_frameIndex].get();
    }
    return nullptr;
}

void DX12SwapChain::present() {
    UINT syncInterval = m_desc.vsync ? 1 : 0;
    UINT flags = m_desc.vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;
    m_swapChain->Present(syncInterval, flags);
}

void DX12SwapChain::resize(uint32_t width, uint32_t height) {
    releaseBackBuffers();

    m_desc.width  = width;
    m_desc.height = height;

    HRESULT hr = m_swapChain->ResizeBuffers(
        m_desc.bufferCount, width, height,
        toDXGIFormat(m_desc.format), 0);
    DX12_CHECK(hr);

    createRTVHeap();
    createBackBuffers();
}

void DX12SwapChain::beginRenderPass(CommandList* cmd) {
    auto* dx12Cmd = static_cast<DX12CommandList*>(cmd);
    auto* cl = dx12Cmd->commandList();

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Transition back buffer to RT
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_backBuffers[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cl->ResourceBarrier(1, &barrier);

    // RTV + DSV handles
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
    rtvHandle.ptr = m_rtvHeap->heap()->GetCPUDescriptorHandleForHeapStart().ptr
                  + static_cast<uint64_t>(m_frameIndex) * m_rtvHeap->descriptorSize();

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
    dsvHandle.ptr = m_dsvHeap->heap()->GetCPUDescriptorHandleForHeapStart().ptr;

    // Clear
    cl->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
    cl->ClearDepthStencilView(dsvHandle,
                               D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                               1.0f, 0, 0, nullptr);

    // Set render targets
    cl->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void DX12SwapChain::endRenderPass(CommandList* cmd) {
    auto* dx12Cmd = static_cast<DX12CommandList*>(cmd);
    auto* cl = dx12Cmd->commandList();

    // Transition back buffer to present
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_backBuffers[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    cl->ResourceBarrier(1, &barrier);
}

} // namespace MulanGeo::Engine
