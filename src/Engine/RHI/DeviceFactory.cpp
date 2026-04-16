/**
 * @file DeviceFactory.cpp
 * @brief RHI设备工厂，根据后端类型创建具体实现
 * @author hxxcxx
 * @date 2026-04-15
 */

#include "Device.h"
#include "Vulkan/VKDevice.h"

namespace MulanGeo::Engine {

std::unique_ptr<RHIDevice> RHIDevice::create(const DeviceCreateInfo& ci) {
    switch (ci.backend) {
    case GraphicsBackend::Vulkan:
        return std::make_unique<VKDevice>(ci);

    // 后续可扩展：
    // case GraphicsBackend::D3D12:
    //     return std::make_unique<D3D12Device>(ci);

    default:
        return nullptr;
    }
}

} // namespace MulanGeo::Engine
