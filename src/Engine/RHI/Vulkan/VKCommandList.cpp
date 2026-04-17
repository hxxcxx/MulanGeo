#include "VKCommandList.h"

namespace MulanGeo::Engine {

VKCommandList::VKCommandList(vk::Device device, uint32_t queueFamilyIndex)
    : m_device(device)
    , m_ownsPool(true)
{
    vk::CommandPoolCreateInfo poolCI;
    poolCI.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolCI.queueFamilyIndex = queueFamilyIndex;

    m_pool = m_device.createCommandPool(poolCI);

    vk::CommandBufferAllocateInfo allocCI;
    allocCI.commandPool        = m_pool;
    allocCI.level              = vk::CommandBufferLevel::ePrimary;
    allocCI.commandBufferCount = 1;

    auto cmdBuffers = m_device.allocateCommandBuffers(allocCI);
    m_cmdBuffer = cmdBuffers[0];
}

VKCommandList::VKCommandList(vk::CommandBuffer externalCmd)
    : m_cmdBuffer(externalCmd)
    , m_ownsPool(false)
{}

VKCommandList::~VKCommandList() {
    if (m_ownsPool && m_pool) {
        m_device.freeCommandBuffers(m_pool, m_cmdBuffer);
        m_device.destroyCommandPool(m_pool);
    }
}

void VKCommandList::begin() {
    if (m_ownsPool) {
        m_device.resetCommandPool(m_pool);
    }

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    m_cmdBuffer.begin(beginInfo);
}

void VKCommandList::end() {
    m_cmdBuffer.end();
}

void VKCommandList::setPipelineState(PipelineState* pso) {
    auto* vkPso = static_cast<VKPipelineState*>(pso);
    m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPso->pipeline());
    m_currentLayout = vkPso->layout();
}

void VKCommandList::setViewport(const Viewport& vp) {
    vk::Viewport viewport;
    viewport.x        = vp.x;
    viewport.y        = vp.y;
    viewport.width    = vp.width;
    viewport.height   = vp.height;
    viewport.minDepth = vp.minDepth;
    viewport.maxDepth = vp.maxDepth;
    m_cmdBuffer.setViewport(0, 1, &viewport);
}

void VKCommandList::setScissorRect(const ScissorRect& rect) {
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(rect.x, rect.y);
    scissor.extent = vk::Extent2D(static_cast<uint32_t>(rect.width),
                                   static_cast<uint32_t>(rect.height));
    m_cmdBuffer.setScissor(0, 1, &scissor);
}

void VKCommandList::setVertexBuffer(uint32_t slot, Buffer* buffer, uint32_t offset) {
    auto* vkBuf = static_cast<VKBuffer*>(buffer);
    vk::Buffer buf = vkBuf->vkBuffer();
    vk::DeviceSize offs = offset;
    m_cmdBuffer.bindVertexBuffers(slot, 1, &buf, &offs);
}

void VKCommandList::setVertexBuffers(uint32_t startSlot, uint32_t count,
                                      Buffer** buffers, uint32_t* offsets) {
    std::vector<vk::Buffer> bufs;
    std::vector<vk::DeviceSize> offs;
    for (uint32_t i = 0; i < count; ++i) {
        bufs.push_back(static_cast<VKBuffer*>(buffers[i])->vkBuffer());
        offs.push_back(offsets ? offsets[i] : 0);
    }
    m_cmdBuffer.bindVertexBuffers(startSlot, bufs, offs);
}

void VKCommandList::setIndexBuffer(Buffer* buffer, uint32_t offset, IndexType type) {
    auto* vkBuf = static_cast<VKBuffer*>(buffer);
    m_cmdBuffer.bindIndexBuffer(vkBuf->vkBuffer(), offset, toVkIndexType(type));
}

void VKCommandList::draw(const DrawAttribs& attribs) {
    m_cmdBuffer.draw(attribs.vertexCount, attribs.instanceCount,
                     attribs.startVertex, attribs.startInstance);
}

void VKCommandList::drawIndexed(const DrawIndexedAttribs& attribs) {
    m_cmdBuffer.drawIndexed(attribs.indexCount, attribs.instanceCount,
                            attribs.startIndex, attribs.baseVertex,
                            attribs.startInstance);
}

void VKCommandList::updateBuffer(Buffer* buffer, uint32_t offset,
                                  uint32_t size, const void* data,
                                  ResourceTransitionMode) {
    auto* vkBuf = static_cast<VKBuffer*>(buffer);
    vkBuf->update(offset, size, data);
}

void VKCommandList::transitionResource(Buffer*, ResourceState) {
    // Vulkan 通过 pipeline barrier 处理，此处简化
}

void VKCommandList::clearColor(float r, float g, float b, float a) {
    // 通过 renderPass 的 clearValue 实现
}

void VKCommandList::clearDepth(float) {
    // 通过 renderPass 的 clearValue 实现
}

void VKCommandList::clearStencil(uint8_t) {
    // 通过 renderPass 的 clearValue 实现
}

void VKCommandList::beginRenderPass(vk::RenderPass renderPass, vk::Framebuffer framebuffer,
                                     uint32_t width, uint32_t height,
                                     const std::array<float, 4>& clearColor,
                                     float clearDepth) {
    vk::ClearValue clearValues[2];
    clearValues[0].color = vk::ClearColorValue(clearColor);
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(clearDepth, 0);

    vk::RenderPassBeginInfo rpBegin;
    rpBegin.renderPass      = renderPass;
    rpBegin.framebuffer     = framebuffer;
    rpBegin.renderArea      = vk::Rect2D({0, 0}, {width, height});
    rpBegin.clearValueCount = 2;
    rpBegin.pClearValues    = clearValues;

    m_cmdBuffer.beginRenderPass(rpBegin, vk::SubpassContents::eInline);
}

void VKCommandList::endRenderPass() {
    m_cmdBuffer.endRenderPass();
}

void VKCommandList::bindDescriptorSet(vk::PipelineLayout layout, vk::DescriptorSet set,
                                       uint32_t firstSet) {
    m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   layout, firstSet, 1, &set, 0, nullptr);
}

} // namespace MulanGeo::Engine
