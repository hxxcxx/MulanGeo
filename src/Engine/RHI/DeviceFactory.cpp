#include "Device.h"
#include "Vulkan/VKDevice.h"
#include "D3D12/DX12Device.h"
#include "D3D11/DX11Device.h"
#include "OpenGL/GLDevice.h"

namespace MulanGeo::Engine {

std::unique_ptr<RHIDevice> RHIDevice::create(const DeviceCreateInfo& ci) {
    switch (ci.backend) {
    case GraphicsBackend::Vulkan:
        return std::make_unique<VKDevice>(ci);

    case GraphicsBackend::D3D12:
        return std::make_unique<DX12Device>(ci);

    case GraphicsBackend::D3D11:
        return std::make_unique<DX11Device>(ci);

    case GraphicsBackend::OpenGL:
        return std::make_unique<GLDevice>(ci);

    default:
        return nullptr;
    }
}

} // namespace MulanGeo::Engine
