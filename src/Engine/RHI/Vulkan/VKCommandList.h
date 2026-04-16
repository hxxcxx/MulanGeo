/**
 * @file VKCommandList.h
 * @brief Vulkan命令列表实现，支持独立与外部buffer两种模式
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../CommandList.h"
#include "VkConvert.h"

namespace MulanGeo::Engine {

class VKCommandList : public CommandList {
public:
    /// 独立模式：自己创建 command pool + buffer
    VKCommandList(vk::Device device, uint32_t queueFamilyIndex)
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

    /// 外部 buffer 模式：引用 frameContext 的 command buffer
    explicit VKCommandList(vk::CommandBuffer externalCmd)
        : m_cmdBuffer(externalCmd)
        , m_ownsPool(false)
    {}

    ~VKCommandList() {
        if (m_ownsPool && m_pool) {
            m_device.freeCommandBuffers(m_pool, m_cmdBuffer);
            m_device.destroyCommandPool(m_pool);
        }
    }

    vk::CommandBuffer cmdBuffer() const { return m_cmdBuffer; }

    // --- 生命周期 ---

    void begin() override {
        if (m_ownsPool) {
            m_device.resetCommandPool(m_pool);
        }

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        m_cmdBuffer.begin(beginInfo);
    }

    void end() override {
        m_cmdBuffer.end();
    }

    // --- 管线状态 ---

    void setPipelineState(PipelineState* pso) override {
        auto* vkPso = static_cast<VKPipelineState*>(pso);
        m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPso->pipeline());
        m_currentLayout = vkPso->layout();
    }

    // --- 视口 / 裁剪 ---

    void setViewport(const Viewport& vp) override {
        vk::Viewport viewport;
        viewport.x        = vp.x;
        viewport.y        = vp.y;
        viewport.width    = vp.width;
        viewport.height   = vp.height;
        viewport.minDepth = vp.minDepth;
        viewport.maxDepth = vp.maxDepth;
        m_cmdBuffer.setViewport(0, 1, &viewport);
    }

    void setScissorRect(const ScissorRect& rect) override {
        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D(rect.x, rect.y);
        scissor.extent = vk::Extent2D(static_cast<uint32_t>(rect.width),
                                       static_cast<uint32_t>(rect.height));
        m_cmdBuffer.setScissor(0, 1, &scissor);
    }

    // --- 缓冲区绑定 ---

    void setVertexBuffer(uint32_t slot, Buffer* buffer,
                         uint32_t offset = 0) override {
        auto* vkBuf = static_cast<VKBuffer*>(buffer);
        vk::Buffer buf = vkBuf->vkBuffer();
        vk::DeviceSize offs = offset;
        m_cmdBuffer.bindVertexBuffers(slot, 1, &buf, &offs);
    }

    void setVertexBuffers(uint32_t startSlot, uint32_t count,
                          Buffer** buffers, uint32_t* offsets) override {
        std::vector<vk::Buffer> bufs;
        std::vector<vk::DeviceSize> offs;
        for (uint32_t i = 0; i < count; ++i) {
            bufs.push_back(static_cast<VKBuffer*>(buffers[i])->vkBuffer());
            offs.push_back(offsets ? offsets[i] : 0);
        }
        m_cmdBuffer.bindVertexBuffers(startSlot, bufs, offs);
    }

    void setIndexBuffer(Buffer* buffer, uint32_t offset = 0,
                        IndexType type = IndexType::UInt32) override {
        auto* vkBuf = static_cast<VKBuffer*>(buffer);
        m_cmdBuffer.bindIndexBuffer(vkBuf->vkBuffer(), offset, toVkIndexType(type));
    }

    // --- 绘制 ---

    void draw(const DrawAttribs& attribs) override {
        m_cmdBuffer.draw(attribs.vertexCount, attribs.instanceCount,
                         attribs.startVertex, attribs.startInstance);
    }

    void drawIndexed(const DrawIndexedAttribs& attribs) override {
        m_cmdBuffer.drawIndexed(attribs.indexCount, attribs.instanceCount,
                                attribs.startIndex, attribs.baseVertex,
                                attribs.startInstance);
    }

    // --- 资源更新 ---

    void updateBuffer(Buffer* buffer, uint32_t offset,
                      uint32_t size, const void* data,
                      ResourceTransitionMode mode =
                          ResourceTransitionMode::Transition) override {
        auto* vkBuf = static_cast<VKBuffer*>(buffer);
        vkBuf->update(offset, size, data);
    }

    // --- 资源状态转换 ---

    void transitionResource(Buffer* buffer,
                            ResourceState newState) override {
        // Vulkan 通过 pipeline barrier 处理，此处简化
        // 实际实现中需要 vkCmdPipelineBarrier
    }

    // --- 清除 ---

    void clearColor(float r, float g, float b, float a) override {
        vk::ClearColorValue clearValue;
        clearValue.float32[0] = r;
        clearValue.float32[1] = g;
        clearValue.float32[2] = b;
        clearValue.float32[3] = a;

        // 通过 renderPass 的 clearValue 实现
        // 实际清除在 beginRenderPass 时自动执行
    }

    void clearDepth(float depth) override {
        // 通过 renderPass 的 clearValue 实现
    }

    void clearStencil(uint8_t stencil) override {
        // 通过 renderPass 的 clearValue 实现
    }

    // --- Vulkan 特有：开始/结束 RenderPass ---

    void beginRenderPass(vk::RenderPass renderPass, vk::Framebuffer framebuffer,
                         uint32_t width, uint32_t height,
                         const std::array<float, 4>& clearColor = {0.15f, 0.15f, 0.15f, 1.0f},
                         float clearDepth = 1.0f) {
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

    void endRenderPass() {
        m_cmdBuffer.endRenderPass();
    }

    vk::PipelineLayout currentLayout() const { return m_currentLayout; }

    /// 绑定 descriptor set 到当前管线
    void bindDescriptorSet(vk::PipelineLayout layout, vk::DescriptorSet set,
                           uint32_t firstSet = 0) {
        m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       layout, firstSet, 1, &set, 0, nullptr);
    }

private:
    vk::Device          m_device;
    vk::CommandPool     m_pool;
    vk::CommandBuffer   m_cmdBuffer;
    vk::PipelineLayout  m_currentLayout;
    bool                m_ownsPool;
};

} // namespace MulanGeo::Engine
