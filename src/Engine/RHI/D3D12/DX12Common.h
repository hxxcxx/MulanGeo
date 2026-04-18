/**
 * @file DX12Common.h
 * @brief D3D12 后端公共头文件，统一 include 与工具类型
 * @author hxxcxx
 * @date 2026-04-18
 */

#pragma once

// Windows 头文件最小化
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// Windows SDK 将 DeviceCapabilities 定义为宏（映射到 DeviceCapabilitiesA/W）
// 取消定义以避免与 MulanGeo::Engine::DeviceCapabilities 冲突
#ifdef DeviceCapabilities
#undef DeviceCapabilities
#endif

#include <cstdio>
#include <cstdint>
#include <cassert>

// D3D12 调试层检查
#ifdef _DEBUG
#define DX12_CHECK(hr)                                                      \
    do {                                                                    \
        if (FAILED(hr)) {                                                   \
            fprintf(stderr, "[DX12 ERROR] HRESULT=0x%08X at %s:%d\n",      \
                    (hr), __FILE__, __LINE__);                               \
        }                                                                   \
    } while (0)
#else
#define DX12_CHECK(hr) (void)(hr)
#endif

namespace MulanGeo::Engine {

using Microsoft::WRL::ComPtr;

/// 安全释放 COM 指针
template<typename T>
inline void safeRelease(T*& ptr) {
    if (ptr) { ptr->Release(); ptr = nullptr; }
}

/// 安全释放 ComPtr（reset 即可）
template<typename T>
inline void safeRelease(ComPtr<T>& ptr) {
    ptr.Reset();
}

} // namespace MulanGeo::Engine
