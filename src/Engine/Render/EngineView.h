/**
 * @file EngineView.h
 * @brief 平台无关的引擎视图 — RHI + Camera + Operator 的整合层
 * @author hxxcxx
 * @date 2026-04-17
 *
 * 设计目标：
 *  - 将所有引擎逻辑（设备初始化、着色器、管线、UBO、帧循环、交互）
 *    封装在一个平台无关的类中
 *  - Qt / Win32 / SDL 等 UI 层只需：
 *      1. 创建 EngineView
 *      2. init(NativeWindowHandle, w, h)
 *      3. resize(w, h) / renderFrame() / handleInput(InputEvent)
 *  - UI 层不再直接接触任何 RHI 类型
 */

#pragma once

#include "../RHI/Device.h"
#include "../RHI/SwapChain.h"
#include "../RHI/RenderTarget.h"
#include "../RHI/Buffer.h"
#include "../RHI/Shader.h"
#include "../RHI/PipelineState.h"
#include "../RHI/VertexLayout.h"
#include "../Scene/Camera.h"
#include "../Interaction/InputEvent.h"
#include "../Interaction/Operator.h"
#include "../Interaction/CameraManipulator.h"
#include "../Window.h"
#include "SceneRenderer.h"
#include "RenderGeometry.h"
#include "../Math/Mat4.h"
#include "../Math/Vec3.h"
#include "../Math/AABB.h"

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>

namespace MulanGeo::Engine {

// ============================================================
// GPU UBO 结构 — 与 shader Common.hlsli 对齐
// ============================================================

struct alignas(16) CameraUBO {
    float view[16];
    float projection[16];
    float viewProjection[16];
    float cameraPos[3];
    float _pad0;
};

struct alignas(16) ObjectUBO {
    float world[16];
    float normalMat[9];
    float _pad1[3];
    uint32_t pickId;
    float _pad2[3];
};

struct alignas(16) LightMaterialUBO {
    float baseColor[3];
    float _pad2;
    float lightDir[3];
    float _pad3;
    float ambientColor[3];
    float _pad4;
    float wireColor[3];
    float _pad5;
};

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

    /// 渲染一帧（由 UI 层的定时器驱动）
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

    // --- 场景收集 ---

    /// 收集回调：每帧渲染前调用以填充 RenderQueue
    using CollectCallback = std::function<void(const Camera&, RenderQueue&)>;

    /// 设置收集回调（由 UIDocument::attachView 调用）
    void setCollector(CollectCallback cb);

    /// 清除收集回调
    void clearCollector();

private:
    void loadShaders();
    void createPSOs();
    void createUBOs();
    void updateCameraUBO();
    void initSceneRenderer();
    void cleanup();

    // --- RHI 资源 ---

    std::unique_ptr<RHIDevice>  m_device;
    ResourcePtr<SwapChain>      m_swapchain;
    ResourcePtr<RenderTarget>   m_renderTarget;
    ResourcePtr<Buffer>         m_stagingBuffer;

    ResourcePtr<Shader>         m_solidVs;
    ResourcePtr<Shader>         m_solidFs;

    ResourcePtr<PipelineState>  m_solidPso;
    VertexLayout                m_vertexLayout;

    ResourcePtr<Buffer>         m_cameraBuffer;
    ResourcePtr<Buffer>         m_objectBuffer;
    ResourcePtr<Buffer>         m_materialBuffer;

    // --- Camera & Interaction ---

    Camera                              m_camera;
    std::unique_ptr<Operator>           m_operator;

    // --- Renderer ---
    std::unique_ptr<SceneRenderer>      m_sceneRenderer;
    RenderQueue                         m_renderQueue;
    CollectCallback                     m_collectCallback;

    // --- 状态 ---

    int  m_width       = 0;
    int  m_height      = 0;
    bool m_initialized = false;
};

} // namespace MulanGeo::Engine
