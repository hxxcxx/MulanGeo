/*
 * Vulkan SwapChain 实现
 */

#pragma once

#include "../SwapChain.h"
#include "VkConvert.h"

namespace MulanGeo::Engine {

class VKSwapChain : public SwapChain {
public:
    struct InitParams {
        vk::Instance       instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device         device;
        VmaAllocator       allocator;
        uint32_t           graphicsQueueFamily;
        uint32_t           presentQueueFamily;
        vk::Queue          graphicsQueue;
        vk::Queue          presentQueue;
        void*              windowHandle;      // HWND on Windows
    };

    VKSwapChain(const SwapChainDesc& desc, const InitParams& params)
        : m_desc(desc), m_params(params)
    {
        createSwapChain();
        createRenderPass();
        createFramebuffers();
        createSyncObjects();
    }

    ~VKSwapChain() {
        cleanup();
    }

    const SwapChainDesc& desc() const override { return m_desc; }

    Texture* currentBackBuffer() override {
        return m_backBuffers[m_currentFrame].get();
    }

    void present() override {
        // 提交命令后 present
        vk::PresentInfoKHR presentInfo;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains    = &m_swapchain;
        presentInfo.pImageIndices  = &m_currentImageIndex;

        m_params.presentQueue.presentKHR(&presentInfo);
        m_currentFrame = (m_currentFrame + 1) % m_desc.bufferCount;
    }

    void resize(uint32_t width, uint32_t height) override {
        m_params.device.waitIdle();
        cleanup();

        m_desc.width  = width;
        m_desc.height = height;

        createSwapChain();
        createRenderPass();
        createFramebuffers();
    }

    // Vulkan 特有
    vk::RenderPass renderPass() const { return m_renderPass; }
    vk::Framebuffer currentFramebuffer() const {
        return m_framebuffers[m_currentImageIndex];
    }

    bool acquireNextImage() {
        auto result = m_params.device.acquireNextImageKHR(
            m_swapchain, UINT64_MAX,
            m_imageAvailableSemaphores[m_currentFrame],
            nullptr);
        if (result.result == vk::Result::eSuccess ||
            result.result == vk::Result::eSuboptimalKHR) {
            m_currentImageIndex = result.value;
            return true;
        }
        return false;
    }

    vk::Semaphore imageAvailableSemaphore() const {
        return m_imageAvailableSemaphores[m_currentFrame];
    }
    vk::Semaphore renderFinishedSemaphore() const {
        return m_renderFinishedSemaphores[m_currentFrame];
    }
    vk::Fence inFlightFence() const {
        return m_inFlightFences[m_currentFrame];
    }

private:
    void createSwapChain() {
        // 查询 swapchain 支持
        auto caps     = m_params.physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
        auto formats  = m_params.physicalDevice.getSurfaceFormatsKHR(m_surface);
        auto modes    = m_params.physicalDevice.getSurfacePresentModesKHR(m_surface);

        // 选择格式
        vk::SurfaceFormatKHR surfaceFormat = formats[0];
        for (auto& f : formats) {
            if (f.format == vk::Format::eB8G8R8A8Srgb &&
                f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                surfaceFormat = f;
                break;
            }
        }

        // 选择 present mode
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo; // VSync
        if (!m_desc.vsync) {
            for (auto& m : modes) {
                if (m == vk::PresentModeKHR::eMailbox) {
                    presentMode = m;
                    break;
                }
                if (m == vk::PresentModeKHR::eImmediate) {
                    presentMode = m;
                }
            }
        }

        // Extent
        vk::Extent2D extent;
        if (caps.currentExtent.width != UINT32_MAX) {
            extent = caps.currentExtent;
        } else {
            extent.width  = (std::min)(std::max(m_desc.width,
                caps.minImageExtent.width), caps.maxImageExtent.width);
            extent.height = (std::min)(std::max(m_desc.height,
                caps.minImageExtent.height), caps.maxImageExtent.height);
        }

        // Image count
        uint32_t imageCount = m_desc.bufferCount;
        if (caps.maxImageCount > 0) {
            imageCount = std::min(imageCount, caps.maxImageCount);
        }

        // Queue families
        uint32_t queueFamilyIndices[] = {
            m_params.graphicsQueueFamily,
            m_params.presentQueueFamily
        };

        vk::SwapchainCreateInfoKHR ci;
        ci.surface          = m_surface;
        ci.minImageCount    = imageCount;
        ci.imageFormat      = surfaceFormat.format;
        ci.imageColorSpace  = surfaceFormat.colorSpace;
        ci.imageExtent      = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
        ci.preTransform     = caps.currentTransform;
        ci.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        ci.presentMode      = presentMode;
        ci.clipped          = true;

        if (m_params.graphicsQueueFamily != m_params.presentQueueFamily) {
            ci.imageSharingMode      = vk::SharingMode::eConcurrent;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices   = queueFamilyIndices;
        } else {
            ci.imageSharingMode = vk::SharingMode::eExclusive;
        }

        auto result = m_params.device.createSwapchainKHR(ci);
        m_swapchain = result;

        m_swapchainImages  = m_params.device.getSwapchainImagesKHR(m_swapchain);
        m_swapchainFormat  = surfaceFormat.format;
        m_swapchainExtent  = extent;

        // 创建 ImageViews
        m_imageViews.resize(m_swapchainImages.size());
        for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
            vk::ImageViewCreateInfo viewCI;
            viewCI.image            = m_swapchainImages[i];
            viewCI.viewType         = vk::ImageViewType::e2D;
            viewCI.format           = m_swapchainFormat;
            viewCI.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
            viewCI.subresourceRange.levelCount     = 1;
            viewCI.subresourceRange.layerCount     = 1;

            m_imageViews[i] = m_params.device.createImageView(viewCI);
        }

        // 深度缓冲
        TextureDesc depthDesc = TextureDesc::depthStencil(extent.width, extent.height,
                                                          TextureFormat::D24_UNorm_S8_UInt,
                                                          "DepthBuffer");
        m_depthTexture = std::make_unique<VKTexture>(depthDesc, m_params.device, m_params.allocator);
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment;
        colorAttachment.format         = m_swapchainFormat;
        colorAttachment.samples        = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentDescription depthAttachment;
        depthAttachment.format         = vk::Format::eD24UnormS8Uint;
        depthAttachment.samples        = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp        = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference colorRef;
        colorRef.attachment = 0;
        colorRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depthRef;
        depthRef.attachment = 1;
        depthRef.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        vk::RenderPassCreateInfo rpCI;
        rpCI.attachmentCount = static_cast<uint32_t>(attachments.size());
        rpCI.pAttachments    = attachments.data();
        rpCI.subpassCount    = 1;
        rpCI.pSubpasses      = &subpass;

        m_renderPass = m_params.device.createRenderPass(rpCI);
    }

    void createFramebuffers() {
        m_framebuffers.resize(m_imageViews.size());
        for (size_t i = 0; i < m_imageViews.size(); ++i) {
            vk::ImageView attachments[] = {
                m_imageViews[i],
                static_cast<VKTexture*>(m_depthTexture.get())->view()
            };

            vk::FramebufferCreateInfo fbCI;
            fbCI.renderPass       = m_renderPass;
            fbCI.attachmentCount  = 2;
            fbCI.pAttachments     = attachments;
            fbCI.width            = m_swapchainExtent.width;
            fbCI.height           = m_swapchainExtent.height;
            fbCI.layers           = 1;

            m_framebuffers[i] = m_params.device.createFramebuffer(fbCI);
        }
    }

    void createSyncObjects() {
        m_imageAvailableSemaphores.resize(m_desc.bufferCount);
        m_renderFinishedSemaphores.resize(m_desc.bufferCount);
        m_inFlightFences.resize(m_desc.bufferCount);

        vk::FenceCreateInfo fenceCI;
        fenceCI.flags = vk::FenceCreateFlagBits::eSignaled;

        for (uint32_t i = 0; i < m_desc.bufferCount; ++i) {
            m_imageAvailableSemaphores[i] = m_params.device.createSemaphore({});
            m_renderFinishedSemaphores[i] = m_params.device.createSemaphore({});
            m_inFlightFences[i]           = m_params.device.createFence(fenceCI);
        }
    }

    void cleanup() {
        for (auto& fb : m_framebuffers)
            m_params.device.destroyFramebuffer(fb);
        m_framebuffers.clear();

        for (auto& iv : m_imageViews)
            m_params.device.destroyImageView(iv);
        m_imageViews.clear();

        if (m_renderPass) {
            m_params.device.destroyRenderPass(m_renderPass);
            m_renderPass = nullptr;
        }

        if (m_swapchain) {
            m_params.device.destroySwapchainKHR(m_swapchain);
            m_swapchain = nullptr;
        }

        m_depthTexture.reset();

        for (uint32_t i = 0; i < m_desc.bufferCount; ++i) {
            if (m_imageAvailableSemaphores[i])
                m_params.device.destroySemaphore(m_imageAvailableSemaphores[i]);
            if (m_renderFinishedSemaphores[i])
                m_params.device.destroySemaphore(m_renderFinishedSemaphores[i]);
            if (m_inFlightFences[i])
                m_params.device.destroyFence(m_inFlightFences[i]);
        }
    }

    SwapChainDesc    m_desc;
    InitParams       m_params;

    vk::SurfaceKHR   m_surface;
    vk::SwapchainKHR m_swapchain;
    std::vector<vk::Image>     m_swapchainImages;
    std::vector<vk::ImageView> m_imageViews;
    vk::Format       m_swapchainFormat;
    vk::Extent2D     m_swapchainExtent;

    vk::RenderPass   m_renderPass;
    std::vector<vk::Framebuffer> m_framebuffers;

    std::unique_ptr<VKTexture> m_depthTexture;
    std::vector<std::unique_ptr<VKTexture>> m_backBuffers;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence>     m_inFlightFences;

    uint32_t m_currentImageIndex = 0;
    uint32_t m_currentFrame      = 0;
};

} // namespace MulanGeo::Engine
