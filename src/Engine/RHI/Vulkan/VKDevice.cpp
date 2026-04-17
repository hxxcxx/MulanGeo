// VMA 实现（必须在 #include <vk_mem_alloc.h> 之前）
#define VMA_IMPLEMENTATION

#include "VKDevice.h"

// Vulkan-Hpp 动态 dispatch 存储（必须在 #include <vulkan/vulkan.hpp> 之后）
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <set>
#include <algorithm>
#include <cstdio>

namespace MulanGeo::Engine {

// ============================================================
// 构造 / 析构
// ============================================================

VKDevice::VKDevice(const CreateInfo& ci) {
    init(ci);
}

VKDevice::VKDevice(const DeviceCreateInfo& ci) {
    CreateInfo vkCI;
    vkCI.enableValidation = ci.enableValidation;
    vkCI.window           = ci.window;
    vkCI.renderConfig     = ci.renderConfig;
    vkCI.appName          = ci.appName;
    init(vkCI);
}

VKDevice::~VKDevice() {
    m_device.waitIdle();

    m_frameContexts.clear();
    m_uploadContext.reset();
    m_descriptorAllocator.reset();
    m_swapChains.clear();

    shutdown();
}

// ============================================================
// Device 信息
// ============================================================

GraphicsBackend VKDevice::backend() const {
    return GraphicsBackend::Vulkan;
}

const DeviceCapabilities& VKDevice::capabilities() const {
    return m_caps;
}

const RenderConfig& VKDevice::renderConfig() const {
    return m_renderConfig;
}

// ============================================================
// 资源创建
// ============================================================

Buffer* VKDevice::createBuffer(const BufferDesc& desc) {
    auto* buf = new VKBuffer(desc, m_allocator);
    if (buf->needsUpload()) {
        m_uploadContext->uploadBufferInit(buf);
    }
    return buf;
}

Texture* VKDevice::createTexture(const TextureDesc& desc) {
    return new VKTexture(desc, m_device, m_allocator);
}

Shader* VKDevice::createShader(const ShaderDesc& desc) {
    return new VKShader(desc, m_device);
}

PipelineState* VKDevice::createPipelineState(const GraphicsPipelineDesc& desc) {
    return new VKPipelineState(desc, m_device);
}

CommandList* VKDevice::createCommandList() {
    return new VKCommandList(m_device, m_graphicsQueueFamily);
}

SwapChain* VKDevice::createSwapChain(const SwapChainDesc& desc) {
    VKSwapChain::InitParams params;
    params.instance            = m_instance;
    params.physicalDevice      = m_physicalDevice;
    params.device              = m_device;
    params.allocator           = m_allocator;
    params.graphicsQueueFamily = m_graphicsQueueFamily;
    params.presentQueueFamily  = m_presentQueueFamily;
    params.graphicsQueue       = m_graphicsQueue;
    params.presentQueue        = m_presentQueue;
    params.surface             = m_surface;

    auto* swapchain = new VKSwapChain(desc, params, m_renderConfig);
    m_swapChains.push_back(swapchain);

    initFrameContexts(m_frameCount);

    return swapchain;
}

Fence* VKDevice::createFence(uint64_t initialValue) {
    return new VKFence(m_device, initialValue);
}

// ============================================================
// 资源销毁
// ============================================================

void VKDevice::destroy(Buffer* resource)         { delete static_cast<VKBuffer*>(resource); }
void VKDevice::destroy(Texture* resource)        { delete static_cast<VKTexture*>(resource); }
void VKDevice::destroy(Shader* resource)         { delete static_cast<VKShader*>(resource); }
void VKDevice::destroy(PipelineState* resource)  { delete static_cast<VKPipelineState*>(resource); }
void VKDevice::destroy(CommandList* resource)    { delete static_cast<VKCommandList*>(resource); }
void VKDevice::destroy(Fence* resource)          { delete static_cast<VKFence*>(resource); }

void VKDevice::destroy(SwapChain* resource) {
    auto it = std::find(m_swapChains.begin(), m_swapChains.end(), resource);
    if (it != m_swapChains.end()) m_swapChains.erase(it);
    delete static_cast<VKSwapChain*>(resource);
}

// ============================================================
// 提交命令
// ============================================================

void VKDevice::executeCommandLists(CommandList** cmdLists, uint32_t count,
                                   Fence* fence, uint64_t fenceValue) {
    std::vector<vk::CommandBuffer> cmdBuffers(count);
    for (uint32_t i = 0; i < count; ++i) {
        cmdBuffers[i] = static_cast<VKCommandList*>(cmdLists[i])->cmdBuffer();
    }

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = count;
    submitInfo.pCommandBuffers    = cmdBuffers.data();

    if (fence) {
        auto* vkFence = static_cast<VKFence*>(fence);
        vk::TimelineSemaphoreSubmitInfo timelineInfo;
        timelineInfo.signalSemaphoreValueCount = 1;
        uint64_t signalValue = fenceValue;
        timelineInfo.pSignalSemaphoreValues = &signalValue;

        vk::Semaphore semaphore = vkFence->semaphore();
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &semaphore;
        submitInfo.pNext                = &timelineInfo;

        m_graphicsQueue.submit(submitInfo);
    } else {
        m_graphicsQueue.submit(submitInfo);
    }
}

void VKDevice::waitIdle() {
    m_device.waitIdle();
}

// ============================================================
// 帧循环
// ============================================================

void VKDevice::beginFrame() {
    auto& frame = currentFrameContext();
    frame.waitForFence();
    frame.resetFence();
    frame.resetCommandBuffer();

    m_descriptorAllocator->resetPools();

    m_frameCmdList = std::make_unique<VKCommandList>(frame.cmdBuffer());

    if (!m_swapChains.empty()) {
        auto* sc = m_swapChains[0];
        sc->acquireNextImage(frame.imageAvailable());
    }
}

CommandList* VKDevice::frameCommandList() {
    return m_frameCmdList.get();
}

void VKDevice::submitAndPresent(SwapChain* swapchain) {
    auto* vkSC  = static_cast<VKSwapChain*>(swapchain);
    auto& frame = currentFrameContext();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    vk::CommandBuffer cmdBuf = static_cast<VKCommandList*>(m_frameCmdList.get())->cmdBuffer();
    submitInfo.pCommandBuffers = &cmdBuf;

    vk::Semaphore waitSemaphores[] = { frame.imageAvailable() };
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;

    vk::Semaphore signalSemaphores[] = { frame.renderFinished() };
    submitInfo.signalSemaphoreCount  = 1;
    submitInfo.pSignalSemaphores     = signalSemaphores;

    m_graphicsQueue.submit(submitInfo, frame.inFlightFence());

    vkSC->presentWithSemaphores(frame.renderFinished());

    m_currentFrame = (m_currentFrame + 1) % m_frameCount;
}

// ============================================================
// Descriptor 绑定
// ============================================================

void VKDevice::bindUniformBuffers(CommandList* cmd, PipelineState* pso,
                                  const UniformBufferBind* binds, uint32_t count) {
    auto* vkPso  = static_cast<VKPipelineState*>(pso);
    auto* vkCmd  = static_cast<VKCommandList*>(cmd);

    vk::DescriptorSet descSet = m_descriptorAllocator->allocate(
        vkPso->descriptorSetLayout());

    for (uint32_t i = 0; i < count; ++i) {
        auto* vkBuf = static_cast<VKBuffer*>(binds[i].buffer);
        m_descriptorAllocator->bindUniformBuffer(
            descSet, binds[i].binding,
            vkBuf->vkBuffer(), binds[i].offset, binds[i].size);
    }

    vkCmd->bindDescriptorSet(vkPso->layout(), descSet);
}

// ============================================================
// 帧上下文初始化
// ============================================================

void VKDevice::initFrameContexts(uint32_t count) {
    m_frameContexts.clear();
    m_frameCount = count;
    for (uint32_t i = 0; i < count; ++i) {
        m_frameContexts.push_back(
            std::make_unique<VKFrameContext>(m_device, m_graphicsQueueFamily));
    }
    m_currentFrame = 0;
    m_frameCmdList = std::make_unique<VKCommandList>(
        currentFrameContext().cmdBuffer());
}

void VKDevice::pickPhysicalDevice(const std::vector<vk::PhysicalDevice>& devices) {
    for (auto& device : devices) {
        auto properties = device.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physicalDevice = device;
            return;
        }
    }
    if (!devices.empty()) {
        m_physicalDevice = devices[0];
    }
}

void VKDevice::createLogicalDevice(bool enableValidation) {
    // Queue families
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_graphicsQueueFamily = i;
        }
        if (m_surface) {
            auto supported = m_physicalDevice.getSurfaceSupportKHR(i, m_surface);
            if (supported) {
                m_presentQueueFamily = i;
            }
        }
    }

    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCIs;
    std::set<uint32_t> uniqueQueues = { m_graphicsQueueFamily };
    if (m_surface) uniqueQueues.insert(m_presentQueueFamily);

    for (uint32_t qf : uniqueQueues) {
        vk::DeviceQueueCreateInfo qCI;
        qCI.queueFamilyIndex = qf;
        qCI.queueCount       = 1;
        qCI.pQueuePriorities = &queuePriority;
        queueCIs.push_back(qCI);
    }

    // Device extensions
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // Features
    vk::PhysicalDeviceFeatures features;
    features.fillModeNonSolid = true;  // wireframe
    features.depthClamp       = true;

    vk::DeviceCreateInfo deviceCI;
    deviceCI.queueCreateInfoCount    = static_cast<uint32_t>(queueCIs.size());
    deviceCI.pQueueCreateInfos       = queueCIs.data();
    deviceCI.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCI.pEnabledFeatures        = &features;

    m_device = m_physicalDevice.createDevice(deviceCI);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);

    m_graphicsQueue = m_device.getQueue(m_graphicsQueueFamily, 0);
    if (m_presentQueueFamily != m_graphicsQueueFamily || m_surface) {
        m_presentQueue = m_device.getQueue(m_presentQueueFamily, 0);
    } else {
        m_presentQueue = m_graphicsQueue;
    }
}

void VKDevice::init(const CreateInfo& ci) {
    m_nativeWindow = ci.window;
    m_renderConfig = ci.renderConfig;
    m_frameCount   = ci.frameCount > 0 ? ci.frameCount : ci.renderConfig.bufferCount;

    // --- Dynamic dispatch loader ---
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

#ifdef _WIN32
    HMODULE vulkanModule = GetModuleHandleW(L"vulkan-1.dll");
    if (!vulkanModule) vulkanModule = LoadLibraryW(L"vulkan-1.dll");
    if (vulkanModule) {
        vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            GetProcAddress(vulkanModule, "vkGetInstanceProcAddr"));
    }
#elif defined(__linux__)
    // Linux: 延迟到 SDL/GLFW/vulkan-1.so dlopen
    // TODO: dlopen("libvulkan.so.1") 加载
#endif

    if (!vkGetInstanceProcAddr) return;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // --- Instance ---
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName   = ci.appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "MulanGeo";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
    };

    // 根据窗口类型添加平台 Surface 扩展
    switch (ci.window.type) {
#ifdef VK_KHR_win32_surface
        case NativeWindowHandle::Type::Win32:
            instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            break;
#endif
#ifdef VK_KHR_xcb_surface
        case NativeWindowHandle::Type::XCB:
            instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
            break;
#endif
        default:
            break;
    }

    std::vector<const char*> instanceLayers;
    if (ci.enableValidation) {
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    vk::InstanceCreateInfo instanceCI;
    instanceCI.pApplicationInfo        = &appInfo;
    instanceCI.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size());
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCI.enabledLayerCount       = static_cast<uint32_t>(instanceLayers.size());
    instanceCI.ppEnabledLayerNames     = instanceLayers.data();

    m_instance = vk::createInstance(instanceCI);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

    // --- Surface（根据平台创建）---
    if (ci.window.valid()) {
        m_surface = createSurface(ci.window);
    }

    // --- Physical Device ---
    auto devices = m_instance.enumeratePhysicalDevices();
    pickPhysicalDevice(devices);

    // --- Device ---
    createLogicalDevice(ci.enableValidation);

    // --- VMA ---
    VmaVulkanFunctions vkFuncs{};
    vkFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    auto vkGetDeviceProcAddrFn = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        vkGetInstanceProcAddr(VkInstance(m_instance), "vkGetDeviceProcAddr"));
    vkFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddrFn;

    VmaAllocatorCreateInfo allocCI{};
    allocCI.physicalDevice   = VkPhysicalDevice(m_physicalDevice);
    allocCI.device           = VkDevice(m_device);
    allocCI.instance         = VkInstance(m_instance);
    allocCI.vulkanApiVersion = VK_API_VERSION_1_3;
    allocCI.pVulkanFunctions = &vkFuncs;

    vmaCreateAllocator(&allocCI, &m_allocator);

    // --- Capabilities ---
    m_caps.backend           = GraphicsBackend::Vulkan;
    auto props               = m_physicalDevice.getProperties();
    m_caps.maxTextureSize    = props.limits.maxImageDimension2D;
    m_caps.maxTextureAniso   = static_cast<uint32_t>(props.limits.maxSamplerAnisotropy);
    m_caps.depthClamp        = true;
    m_caps.geometryShader    = true;
    m_caps.computeShader     = true;

    // --- 私有组件 ---
    m_uploadContext = std::make_unique<VKUploadContext>(
        m_device, m_allocator, m_graphicsQueueFamily, m_graphicsQueue);

    m_descriptorAllocator = std::make_unique<VKDescriptorAllocator>(m_device);

    // FrameContext 在 createSwapChain 时初始化（需要知道 swapchain image count）
}

vk::SurfaceKHR VKDevice::createSurface(const NativeWindowHandle& window) {
    switch (window.type) {
#ifdef _WIN32
    case NativeWindowHandle::Type::Win32: {
        vk::Win32SurfaceCreateInfoKHR ci;
        ci.hinstance = reinterpret_cast<HINSTANCE>(window.win32.hInstance);
        ci.hwnd      = reinterpret_cast<HWND>(window.win32.hWnd);
        return m_instance.createWin32SurfaceKHR(ci);
    }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    case NativeWindowHandle::Type::XCB: {
        vk::XcbSurfaceCreateInfoKHR ci;
        ci.connection = reinterpret_cast<xcb_connection_t*>(window.xcb.connection);
        ci.window     = static_cast<xcb_window_t>(window.xcb.window);
        return m_instance.createXcbSurfaceKHR(ci);
    }
#endif

    default:
        return nullptr;
    }
}

void VKDevice::shutdown() {
    if (m_allocator) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = nullptr;
    }

    if (m_surface) {
        m_instance.destroySurfaceKHR(m_surface);
        m_surface = nullptr;
    }

    if (m_device) {
        m_device.destroy();
        m_device = nullptr;
    }

    if (m_instance) {
        m_instance.destroy();
        m_instance = nullptr;
    }
}

} // namespace MulanGeo::Engine
