/**
 * @file VKPipelineState.h
 * @brief Vulkan管线状态实现
 * @author hxxcxx
 * @date 2026-04-15
 */

#pragma once

#include "../PipelineState.h"
#include "VkConvert.h"

namespace MulanGeo::Engine {

class VKPipelineState : public PipelineState {
public:
    VKPipelineState(const GraphicsPipelineDesc& desc, vk::Device device)
        : m_desc(desc), m_device(device)
    {}

    ~VKPipelineState() {
        if (m_pipeline) m_device.destroyPipeline(m_pipeline);
        if (m_layout)   m_device.destroyPipelineLayout(m_layout);
        if (m_descriptorSetLayout) m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);
    }

    const GraphicsPipelineDesc& desc() const override { return m_desc; }

    vk::Pipeline pipeline() const { return m_pipeline; }
    vk::PipelineLayout layout() const { return m_layout; }

    // 延迟构建 — 需要 renderPass 信息
    void build(vk::RenderPass renderPass, uint32_t subpass = 0) {
        if (m_pipeline) return; // 已构建

        // --- Descriptor Set Layout (3 个 uniform buffer) ---
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        // Camera (b0)
        bindings.push_back({0, vk::DescriptorType::eUniformBuffer,
                            1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment});
        // Object (b1)
        bindings.push_back({1, vk::DescriptorType::eUniformBuffer,
                            1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment});
        // Material (b2)
        bindings.push_back({2, vk::DescriptorType::eUniformBuffer,
                            1, vk::ShaderStageFlagBits::eFragment});

        vk::DescriptorSetLayoutCreateInfo dslCI;
        dslCI.bindingCount = static_cast<uint32_t>(bindings.size());
        dslCI.pBindings    = bindings.data();

        auto dslResult = m_device.createDescriptorSetLayout(dslCI);
        m_descriptorSetLayout = dslResult;

        // --- Push Constant Range (暂不使用，预留) ---

        // --- Pipeline Layout ---
        vk::PipelineLayoutCreateInfo plCI;
        plCI.setLayoutCount         = 1;
        plCI.pSetLayouts            = &m_descriptorSetLayout;
        m_layout = m_device.createPipelineLayout(plCI);

        // --- Shader Stages ---
        std::vector<vk::PipelineShaderStageCreateInfo> stages;
        auto* vkVs = static_cast<VKShader*>(m_desc.vs);
        auto* vkPs = static_cast<VKShader*>(m_desc.ps);

        if (vkVs) stages.push_back(vkVs->stageCreateInfo());
        if (vkPs) stages.push_back(vkPs->stageCreateInfo());

        // --- Vertex Input ---
        auto vertexInputState = buildVertexInputState();

        // --- Input Assembly ---
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = toVkTopology(m_desc.topology);
        inputAssembly.primitiveRestartEnable = false;

        // --- Rasterization ---
        vk::PipelineRasterizationStateCreateInfo raster;
        raster.cullMode              = toVkCullMode(m_desc.cullMode);
        raster.frontFace             = toVkFrontFace(m_desc.frontFace);
        raster.polygonMode           = toVkPolygonMode(m_desc.fillMode);
        raster.lineWidth             = 1.0f;
        raster.depthClampEnable      = false;
        raster.rasterizerDiscardEnable = false;

        // --- Multisample ---
        vk::PipelineMultisampleStateCreateInfo multisample;
        multisample.sampleShadingEnable = false;
        multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

        // --- Depth/Stencil ---
        vk::PipelineDepthStencilStateCreateInfo depthStencil;
        depthStencil.depthTestEnable   = m_desc.depthStencil.depthEnable;
        depthStencil.depthWriteEnable  = m_desc.depthStencil.depthWrite;
        depthStencil.depthCompareOp    = toVkCompareOp(m_desc.depthStencil.depthFunc);
        depthStencil.stencilTestEnable = m_desc.depthStencil.stencilEnable;
        if (m_desc.depthStencil.stencilEnable) {
            depthStencil.front.failOp      = toVkStencilOp(m_desc.depthStencil.frontFace.failOp);
            depthStencil.front.passOp      = toVkStencilOp(m_desc.depthStencil.frontFace.passOp);
            depthStencil.front.depthFailOp = toVkStencilOp(m_desc.depthStencil.frontFace.depthFailOp);
            depthStencil.front.compareOp   = toVkCompareOp(m_desc.depthStencil.frontFace.func);
            depthStencil.back  = depthStencil.front;
        }

        // --- Blend ---
        vk::PipelineColorBlendAttachmentState colorBlend;
        const auto& rt0 = m_desc.blend.renderTargets[0];
        colorBlend.blendEnable    = rt0.blendEnable;
        colorBlend.srcColorBlendFactor  = toVkBlendFactor(rt0.srcBlend);
        colorBlend.dstColorBlendFactor  = toVkBlendFactor(rt0.dstBlend);
        colorBlend.colorBlendOp         = toVkBlendOp(rt0.blendOp);
        colorBlend.srcAlphaBlendFactor  = toVkBlendFactor(rt0.srcBlendAlpha);
        colorBlend.dstAlphaBlendFactor  = toVkBlendFactor(rt0.dstBlendAlpha);
        colorBlend.alphaBlendOp         = toVkBlendOp(rt0.blendOpAlpha);
        colorBlend.colorWriteMask       = vk::ColorComponentFlagBits(rt0.writeMask);

        vk::PipelineColorBlendStateCreateInfo blendCI;
        blendCI.attachmentCount = 1;
        blendCI.pAttachments    = &colorBlend;

        // --- Dynamic State ---
        vk::DynamicState dynamicStates[] = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };
        vk::PipelineDynamicStateCreateInfo dynamicCI;
        dynamicCI.dynamicStateCount = 2;
        dynamicCI.pDynamicStates    = dynamicStates;

        // --- Viewport / Scissor (dynamic) ---
        vk::PipelineViewportStateCreateInfo viewport;
        viewport.viewportCount = 1;
        viewport.scissorCount  = 1;

        // --- Graphics Pipeline ---
        vk::GraphicsPipelineCreateInfo gpCI;
        gpCI.stageCount          = static_cast<uint32_t>(stages.size());
        gpCI.pStages             = stages.data();
        gpCI.pVertexInputState   = &vertexInputState;
        gpCI.pInputAssemblyState = &inputAssembly;
        gpCI.pViewportState      = &viewport;
        gpCI.pRasterizationState = &raster;
        gpCI.pMultisampleState   = &multisample;
        gpCI.pDepthStencilState  = &depthStencil;
        gpCI.pColorBlendState    = &blendCI;
        gpCI.pDynamicState       = &dynamicCI;
        gpCI.layout              = m_layout;
        gpCI.renderPass          = renderPass;
        gpCI.subpass             = subpass;

        m_pipeline = m_device.createGraphicsPipeline(nullptr, gpCI).value;
    }

    vk::DescriptorSetLayout descriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    vk::PipelineVertexInputStateCreateInfo buildVertexInputState() {
        // 从 VertexLayout 构建 Vulkan 顶点输入描述
        m_bindingDescriptions.clear();
        m_attributeDescriptions.clear();

        const auto& layout = m_desc.vertexLayout;
        if (layout.empty()) {
            vk::PipelineVertexInputStateCreateInfo vi;
            return vi;
        }

        // 收集所有 buffer slot，按 slot 分组
        std::unordered_map<uint32_t, uint16_t> slotStrides;
        for (uint8_t i = 0; i < layout.attrCount(); ++i) {
            const auto& attr = layout[i];
            slotStrides[attr.bufferSlot] = layout.stride();
        }

        for (auto& [slot, stride] : slotStrides) {
            m_bindingDescriptions.push_back({slot, stride, vk::VertexInputRate::eVertex});
        }

        for (uint8_t i = 0; i < layout.attrCount(); ++i) {
            const auto& attr = layout[i];
            m_attributeDescriptions.push_back({
                i,                                          // location
                attr.bufferSlot,                            // binding
                vertexFormatToVk(attr.format),              // format
                attr.offset                                 // offset
            });
        }

        vk::PipelineVertexInputStateCreateInfo vi;
        vi.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_bindingDescriptions.size());
        vi.pVertexBindingDescriptions      = m_bindingDescriptions.data();
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
        vi.pVertexAttributeDescriptions    = m_attributeDescriptions.data();
        return vi;
    }

    GraphicsPipelineDesc m_desc;
    vk::Device           m_device;
    vk::Pipeline         m_pipeline;
    vk::PipelineLayout   m_layout;
    vk::DescriptorSetLayout m_descriptorSetLayout;

    // 保持生命周期（buildVertexInputState 中使用）
    std::vector<vk::VertexInputBindingDescription>   m_bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> m_attributeDescriptions;
};

} // namespace MulanGeo::Engine
