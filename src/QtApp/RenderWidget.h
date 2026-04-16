/**
 * @file RenderWidget.h
 * @brief 通用渲染控件，仅依赖 RHI 接口，无后端 specifics
 * @author hxxcxx
 * @date 2026-04-16
 *
 * 职责：
 * - 通过 RHIDevice::create() 创建设备（工厂模式）
 * - 管理资源生命周期（Shader / PSO / UBO）
 * - 标准帧循环：beginFrame → renderPass → draw → submitAndPresent
 * - 鼠标交互 → Camera
 */

#pragma once

#include <QWidget>
#include <QTimer>

#include <MulanGeo/Engine/RHI/Device.h>
#include <MulanGeo/Engine/RHI/SwapChain.h>
#include <MulanGeo/Engine/RHI/Buffer.h>
#include <MulanGeo/Engine/RHI/Shader.h>
#include <MulanGeo/Engine/RHI/PipelineState.h>
#include <MulanGeo/Engine/RHI/VertexLayout.h>
#include <MulanGeo/Engine/Scene/Camera.h>
#include <MulanGeo/Engine/Window.h>
#include <MulanGeo/Engine/Math/Mat4.h>
#include <MulanGeo/Engine/Math/Vec3.h>

#include <memory>
#include <vector>

namespace MulanGeo::IO {
struct ImportResult;
}

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

struct alignas(16) MaterialUBO {
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
// RenderWidget
// ============================================================

class RenderWidget : public QWidget {
    Q_OBJECT

public:
    explicit RenderWidget(QWidget* parent = nullptr);
    ~RenderWidget();

    /// 加载 mesh 数据到 GPU
    void loadMesh(const MulanGeo::IO::ImportResult& result);

    /// 请求渲染下一帧
    void requestFrame();

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void paintEvent(QPaintEvent*) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    /// 初始化 RHI 设备和资源
    bool initRHI();

    /// 加载 SPIR-V 着色器
    void loadShaders();

    /// 创建管线状态
    void createPSOs();

    /// 创建 UBO 缓冲区
    void createUBOs();

    /// 更新 Camera UBO
    void updateCameraUBO();

    /// 渲染一帧
    void renderFrame();

    /// 绘制所有 mesh
    void drawMeshes();

    /// 释放 GPU 资源
    void cleanup();

    // --------------------------------------------------------
    // RHI 资源（后端无关）
    // --------------------------------------------------------

    std::unique_ptr<MulanGeo::Engine::RHIDevice>  m_device;
    MulanGeo::Engine::SwapChain*                  m_swapchain  = nullptr;

    // Shader
    MulanGeo::Engine::Shader*                     m_solidVs    = nullptr;
    MulanGeo::Engine::Shader*                     m_solidFs    = nullptr;

    // PSO
    MulanGeo::Engine::PipelineState*              m_solidPso   = nullptr;
    MulanGeo::Engine::VertexLayout                m_vertexLayout;

    // UBO
    MulanGeo::Engine::Buffer*                     m_cameraBuffer   = nullptr;
    MulanGeo::Engine::Buffer*                     m_objectBuffer   = nullptr;
    MulanGeo::Engine::Buffer*                     m_materialBuffer = nullptr;

    // Camera
    MulanGeo::Engine::Camera                      m_camera;
    QPoint                                        m_lastMousePos;

    // GPU mesh 数据
    struct GpuMesh {
        MulanGeo::Engine::Buffer* vertexBuffer = nullptr;
        MulanGeo::Engine::Buffer* indexBuffer  = nullptr;
        uint32_t vertexCount = 0;
        uint32_t indexCount  = 0;
    };
    std::vector<GpuMesh> m_gpuMeshes;

    QTimer*  m_timer       = nullptr;
    bool     m_initialized = false;
};
