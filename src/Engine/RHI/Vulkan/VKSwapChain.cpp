#include "VKSwapChain.h"
#include "VKTexture.h"

#include <algorithm>

namespace MulanGeo::Engine {

VKSwapChain::VKSwapChain(const SwapChainDesc& desc, const InitParams& params,
                         const RenderConfig& renderConfig)
    : m_desc(desc), m_params(params), m_surface(params.surface)
    , m_renderConfig(renderConfig)
{
    createSwapChain();
    createRenderPass();
    createFramebuffers();
}

VKSwapChain::~VKSwapChain() {
    cleanup();
}

bool VKSwapChain::acquireNextImage(vk::Semaphore imageAvailable) {
    auto result = m_params.device.acquireNextImageKHR(
        m_swapchain, UINT64_MAX, imageAvailable, nullptr);
    if (result.result == vk::Result::eSuccess ||
        result.result == vk::Result::eSuboptimalKHR) {
        m_currentImageIndex = result.value;
        return true;
    }
    return false;
}

void VKSwapChain::presentWithSemaphores(vk::Semaphore renderFinished) {
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &renderFinished;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_currentImageIndex;
    m_params.presentQueue.presentKHR(&presentInfo);
}

void VKSwapChain::present() {
    vk::PresentInfoKHR presentInfo;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = &m_swapchain;
    presentInfo.pImageIndices  = &m_currentImageIndex;
    m_params.presentQueue.presentKHR(&presentInfo);
}

void VKSwapChain::beginRenderPass(CommandList* cmd) {
    auto* vkCmd = static_cast<VKCommandList*>(cmd);
    auto& cc = m_renderConfig.clearColor;
    vkCmd->beginRenderPass(
        m_renderPass,
        m_framebuffers[m_currentImageIndex],
        m_swapchainExtent.width,
        m_swapchainExtent.height,
        { cc[0], cc[1], cc[2], cc[3] },
        1.0f);
}

void VKSwapChain::endRenderPass(CommandList* cmd) {
    auto* vkCmd = static_cast<VKCommandList*>(cmd);
    vkCmd->endRenderPass();
}

void VKSwapChain::resize(uint32_t width, uint32_t height) {
    m_params.device.waitIdle();
    cleanup();

    m_desc.width  = width;
    m_desc.height = height;

    createSwapChain();
    createRenderPass();
    createFramebuffers();
}

// --------------------------------------------------------
// SwapChain 创建
// --------------------------------------------------------

void VKSwapChain::createSwapChain() {
    auto caps     = m_params.physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
    auto formats  = m_params.physicalDevice.getSurfaceFormatsKHR(m_surface);
    auto modes    = m_params.physicalDevice.getSurfacePresentModesKHR(m_surface);

    vk::SurfaceFormatKHR surfaceFormat = formats[0];
    for (auto& f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb &&
            f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            surfaceFormat = f;
            break;
        }
    }

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
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

    vk::Extent2D extent;
    if (caps.currentExtent.width != UINT32_MAX) {
        extent = caps.currentExtent;
    } else {
        extent.width  = (std::min)(std::max(m_desc.width,
            caps.minImageExtent.width), caps.maxImageExtent.width);
        extent.height = (std::min)(std::max(m_desc.height,
            caps.minImageExtent.height), caps.maxImageExtent.height);
    }

    uint32_t imageCount = m_desc.bufferCount;
    if (caps.maxImageCount > 0) {
        imageCount = std::min(imageCount, caps.maxImageCount);
    }

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

    m_swapchain = m_params.device.createSwapchainKHR(ci);

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

void VKSwapChain::createRenderPass() {
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

void VKSwapChain::createFramebuffers() {
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

void VKSwapChain::cleanup() {
    for (auto& fb : m_framebuffers)
        m_params.device.destroyFramebuffer(fb);
    m_framebuffers.clear();

    for (auto& iv : m_imageViews)
        m_params.device.destroyImageView(iv);
    m_imageViews.clear();

    m_backBuffers.clear();
    m_depthTexture.reset();

    if (m_renderPass) {
        m_params.device.destroyRenderPass(m_renderPass);
        m_renderPass = nullptr;
    }

    if (m_swapchain) {
        m_params.device.destroySwapchainKHR(m_swapchain);
        m_swapchain = nullptr;
    }
}

} // namespace MulanGeo::Engine
