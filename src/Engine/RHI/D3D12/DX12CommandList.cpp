/**
 * @file DX12CommandList.cpp
 * @brief D3D12 命令列表实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#include "DX12CommandList.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12PipelineState.h"
#include "DX12Convert.h"

namespace MulanGeo::Engine {

DX12CommandList::DX12CommandList(ID3D12Device* device,
                                 ID3D12CommandAllocator* allocator)
    : m_ownsCmdList(true)
{
    HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           allocator, nullptr,
                                           IID_PPV_ARGS(&m_cmdList));
    DX12_CHECK(hr);
    m_cmdList->Close();
}

DX12CommandList::DX12CommandList(ID3D12GraphicsCommandList* existingCmdList)
    : m_ownsCmdList(false)
{
    if (existingCmdList) {
        m_cmdList = existingCmdList;
    }
    // nullptr 是合法的 — 用于创建占位 wrapper，后续通过 setCommandList 设置
}

DX12CommandList::~DX12CommandList() {
    if (!m_ownsCmdList) {
        m_cmdList.Detach();  // 不 Release 外部的 cmd list
    }
}

void DX12CommandList::setCommandList(ID3D12GraphicsCommandList* cmdList) {
    if (!m_ownsCmdList) {
        m_cmdList.Detach();
    }
    m_cmdList = cmdList;
    m_ownsCmdList = false;
}

void DX12CommandList::begin() {
    // 帧循环模式：cmd list 已由 DX12FrameContext::resetCommandAllocator() Reset
    // 独立模式：需要外部 Reset allocator 后调用
}

void DX12CommandList::end() {
    m_cmdList->Close();
}

void DX12CommandList::setPipelineState(PipelineState* pso) {
    auto* dx12Pso = static_cast<DX12PipelineState*>(pso);
    m_cmdList->SetPipelineState(dx12Pso->pipeline());
    m_cmdList->SetGraphicsRootSignature(dx12Pso->rootSignature());
    m_cmdList->IASetPrimitiveTopology(toDX12Topology(pso->desc().topology));
}

void DX12CommandList::setViewport(const Viewport& vp) {
    D3D12_VIEWPORT d3dVp = { vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth };
    m_cmdList->RSSetViewports(1, &d3dVp);
}

void DX12CommandList::setScissorRect(const ScissorRect& rect) {
    D3D12_RECT d3dRect = { rect.x, rect.y,
                           rect.x + rect.width, rect.y + rect.height };
    m_cmdList->RSSetScissorRects(1, &d3dRect);
}

void DX12CommandList::setVertexBuffer(uint32_t slot, Buffer* buffer, uint32_t offset) {
    auto* dx12Buf = static_cast<DX12Buffer*>(buffer);
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = dx12Buf->gpuAddress() + offset;
    vbv.SizeInBytes    = buffer->size() - offset;
    vbv.StrideInBytes  = 0;  // 由 PSO 的 Input Layout 决定
    // 注意：DX12 不从 VBV 取 stride，但我们设一个合理的值
    // 从当前 PSO 的 vertexLayout 获取 stride — 这里简化处理
    vbv.StrideInBytes = buffer->size();  // 会在 setVertexBuffers 中正确设置
    m_cmdList->IASetVertexBuffers(slot, 1, &vbv);
}

void DX12CommandList::setVertexBuffers(uint32_t startSlot, uint32_t count,
                                        Buffer** buffers, uint32_t* offsets) {
    D3D12_VERTEX_BUFFER_VIEW vbvs[16] = {};
    for (uint32_t i = 0; i < count && i < 16; ++i) {
        auto* dx12Buf = static_cast<DX12Buffer*>(buffers[i]);
        vbvs[i].BufferLocation = dx12Buf->gpuAddress() + (offsets ? offsets[i] : 0);
        vbvs[i].SizeInBytes    = buffers[i]->size() - (offsets ? offsets[i] : 0);
        vbvs[i].StrideInBytes  = 0;  // 由 PSO Input Layout 决定
    }
    m_cmdList->IASetVertexBuffers(startSlot, count, vbvs);
}

void DX12CommandList::setIndexBuffer(Buffer* buffer, uint32_t offset,
                                      IndexType type) {
    auto* dx12Buf = static_cast<DX12Buffer*>(buffer);
    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = dx12Buf->gpuAddress() + offset;
    ibv.SizeInBytes    = buffer->size() - offset;
    ibv.Format         = (type == IndexType::UInt16)
                             ? DXGI_FORMAT_R16_UINT
                             : DXGI_FORMAT_R32_UINT;
    m_cmdList->IASetIndexBuffer(&ibv);
}

void DX12CommandList::draw(const DrawAttribs& attribs) {
    m_cmdList->DrawInstanced(attribs.vertexCount, attribs.instanceCount,
                             attribs.startVertex, attribs.startInstance);
}

void DX12CommandList::drawIndexed(const DrawIndexedAttribs& attribs) {
    m_cmdList->DrawIndexedInstanced(attribs.indexCount, attribs.instanceCount,
                                    attribs.startIndex, attribs.baseVertex,
                                    attribs.startInstance);
}

void DX12CommandList::updateBuffer(Buffer* buffer, uint32_t offset,
                                    uint32_t size, const void* data,
                                    ResourceTransitionMode mode) {
    auto* dx12Buf = static_cast<DX12Buffer*>(buffer);
    dx12Buf->update(offset, size, data);
}

void DX12CommandList::transitionResource(Buffer* buffer, ResourceState newState) {
    auto* dx12Buf = static_cast<DX12Buffer*>(buffer);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12Buf->resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;  // 简化
    barrier.Transition.StateAfter  = toDX12ResourceStates(newState);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);
}

void DX12CommandList::transitionResource(Texture* texture, ResourceState newState) {
    auto* dx12Tex = static_cast<DX12Texture*>(texture);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12Tex->resource();
    barrier.Transition.StateBefore = dx12Tex->state();
    barrier.Transition.StateAfter  = toDX12ResourceStates(newState);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);
    dx12Tex->setState(toDX12ResourceStates(newState));
}

void DX12CommandList::copyTextureToBuffer(Texture* src, Buffer* dst) {
    auto* dx12Tex = static_cast<DX12Texture*>(src);
    auto* dx12Buf = static_cast<DX12Buffer*>(dst);

    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource   = dx12Tex->resource();
    barriers[0].Transition.StateBefore = dx12Tex->state();
    barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.Subresource = 0;
    m_cmdList->ResourceBarrier(1, &barriers[0]);

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.Type            = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.pResource       = dx12Tex->resource();
    srcLoc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.Type          = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLoc.pResource     = dx12Buf->resource();
    // 需要设备来 GetCopyableFootprints — 简化处理
    // 实际实现中需要计算 footprint

    // 简化：使用 buffer 到 buffer 拷贝
    m_cmdList->CopyResource(dx12Buf->resource(), dx12Tex->resource());

    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.StateAfter  = dx12Tex->state();
    m_cmdList->ResourceBarrier(1, &barriers[0]);
}

void DX12CommandList::clearColor(float r, float g, float b, float a) {
    // 由 SwapChain/RenderTarget 在 beginRenderPass 中处理
}

void DX12CommandList::clearDepth(float depth) {
    // 由 SwapChain/RenderTarget 在 beginRenderPass 中处理
}

void DX12CommandList::clearStencil(uint8_t stencil) {
    // 由 SwapChain/RenderTarget 在 beginRenderPass 中处理
}

} // namespace MulanGeo::Engine
