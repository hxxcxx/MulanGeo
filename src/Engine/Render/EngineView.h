/**
 * @file EngineView.h
 * @brief 平台无关的引擎视图 — Device + Camera + Operator 的整合层
 * @author hxxcxx
 * @date 2026-04-17
 *
 * 职责：
 *  - GPU 设备 / SwapChain / RenderTarget 生命周期
 *  - 帧循环调度（收集 → 排序 → 渲染 → 提交）
 *  - 输入 → Operator → Camera
 *
 * 不负责：Shader/PSO/UBO 管理（归 SceneRenderer）
 */

#pragma once

#include "../RHI/Device.h"
#include "../RHI/SwapChain.h"
#include "../RHI/RenderTarget.h"
#include "../RHI/Buffer.h"
#include "../Scene/Camera.h"
#include "../Scene/Scene.h"
#include "../Scene/CullVisitor.h"
#include "../Interaction/InputEvent.h"
#include "../Interaction/Operator.h"
#include "../Interaction/CameraManipulator.h"
#include "../Window.h"
#include "SceneRenderer.h"
#include "RenderGeometry.h"
#include "../Math/Math.h"
#include "../Math/AABB.h"

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace MulanGeo::Engine {

// ============================================================
// EngineView — 引擎视图
// ============================================================

class EngineView {
public:
    EngineView();
    ~EngineView();

    EngineView(const EngineView&) = delete;
    EngineView& operator=(const EngineView&) = delete;

    // --- 生命周期 ---

    /// 初始化（窗口第一次显示时调用）
    bool init(const NativeWindowHandle& window, int width, int height);

    /// 离屏初始化（无窗口，渲染到纹理）
    bool initOffscreen(int width, int height);

    /// 窗口大小改变
    void resize(int width, int height);

    /// 释放所有 GPU 资源
    void shutdown();

    bool isInitialized() const { return m_initialized; }

    // --- 渲染 ---

    /// 渲染一帧
    void renderFrame();

    /// 将 color 纹理回读到 CPU（仅离屏模式）
    /// @param pixels 输出 RGBA 像素数据（自动 resize）
    /// @return true=成功
    bool readbackPixels(std::vector<uint8_t>& pixels);

    // --- 输入 ---

    /// 处理平台无关输入事件
    void handleInput(const InputEvent& event);

    // --- Operator 管理 ---

    /// 设置当前操作器（nullptr 恢复默认 CameraManipulator）
    void setOperator(std::unique_ptr<Operator> op);

    /// 获取当前操作器
    Operator* currentOperator() const { return m_operator.get(); }

    /// 获取离屏渲染目标（nullptr 表示非离屏模式）
    RenderTarget* renderTarget() const { return m_renderTarget.get(); }

    // --- Camera ---

    Camera&       camera()       { return m_camera; }
    const Camera& camera() const { return m_camera; }

    // --- 场景 ---

    /// 设置渲染场景（接管每帧收集逻辑：updateWorldTransforms + CullVisitor）
    void setScene(Scene* scene);

    /// 清除场景引用
    void clearScene();
private:
    void initSceneRenderer();
    void cleanup();

    // --- RHI 资源 ---
    std::unique_ptr<RHIDevice>  m_device;
    ResourcePtr<SwapChain>      m_swapchain;
    ResourcePtr<RenderTarget>   m_renderTarget;
    ResourcePtr<Buffer>         m_stagingBuffer;

    // --- Camera & Interaction ---

    Camera                              m_camera;
    std::unique_ptr<Operator>           m_operator;

    // --- Renderer ---
    std::unique_ptr<SceneRenderer>      m_sceneRenderer;
    RenderQueue                         m_renderQueue;
    Scene*                              m_scene = nullptr;

    // --- 状态 ---

    int  m_width       = 0;
    int  m_height      = 0;
    bool m_initialized = false;
};

} // namespace MulanGeo::Engine
