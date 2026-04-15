/*
 * Vulkan Device 实现
 */

#include "VKDevice.h"

#include <set>

namespace MulanGeo::Engine {

void VKDevice::init(const CreateInfo& ci) {
    m_windowHandle = ci.windowHandle;

    // --- Dynamic dispatch loader ---
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

#ifdef _WIN32
    HMODULE vulkanModule = GetModuleHandleW(L"vulkan-1.dll");
    if (!vulkanModule) vulkanModule = LoadLibraryW(L"vulkan-1.dll");
    if (vulkanModule) {
        vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            GetProcAddress(vulkanModule, "vkGetInstanceProcAddr"));
    }
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
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

    std::vector<const char*> instanceLayers;
    if (ci.enableValidation) {
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    vk::InstanceCreateInfo instanceCI;
    instanceCI.pApplicationInfo       = &appInfo;
    instanceCI.enabledExtensionCount  = static_cast<uint32_t>(instanceExtensions.size());
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCI.enabledLayerCount       = static_cast<uint32_t>(instanceLayers.size());
    instanceCI.ppEnabledLayerNames     = instanceLayers.data();

    m_instance = vk::createInstance(instanceCI);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

    // --- Surface ---
    if (ci.windowHandle) {
#ifdef _WIN32
        vk::Win32SurfaceCreateInfoKHR surfaceCI;
        surfaceCI.hinstance = GetModuleHandleW(nullptr);
        surfaceCI.hwnd      = static_cast<HWND>(ci.windowHandle);
        m_surface = m_instance.createWin32SurfaceKHR(surfaceCI);
#endif
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

void VKDevice::uploadBufferData(VKBuffer* buffer) {
    VkBufferCreateInfo stagingCI{};
    stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingCI.size  = buffer->desc().size;
    stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAlloc{};
    stagingAlloc.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAlloc.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                       | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingInfo;

    vmaCreateBuffer(m_allocator, &stagingCI, &stagingAlloc,
                    &stagingBuffer, &stagingAllocation, &stagingInfo);

    memcpy(stagingInfo.pMappedData, buffer->pendingData(), buffer->desc().size);

    vk::CommandPoolCreateInfo poolCI;
    poolCI.flags            = vk::CommandPoolCreateFlagBits::eTransient;
    poolCI.queueFamilyIndex = m_graphicsQueueFamily;
    auto pool = m_device.createCommandPool(poolCI);

    vk::CommandBufferAllocateInfo allocCI;
    allocCI.commandPool        = pool;
    allocCI.level              = vk::CommandBufferLevel::ePrimary;
    allocCI.commandBufferCount = 1;
    auto cmdBuffers = m_device.allocateCommandBuffers(allocCI);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmdBuffers[0].begin(beginInfo);

    vk::BufferCopy copyRegion;
    copyRegion.size = buffer->desc().size;
    cmdBuffers[0].copyBuffer(vk::Buffer(stagingBuffer), buffer->vkBuffer(), 1, &copyRegion);

    cmdBuffers[0].end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmdBuffers[0];
    m_graphicsQueue.submit(submitInfo);
    m_graphicsQueue.waitIdle();

    m_device.freeCommandBuffers(pool, cmdBuffers);
    m_device.destroyCommandPool(pool);

    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
    buffer->markUploaded();
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
