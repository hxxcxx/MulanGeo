/**
 * @file VulkanRenderWidget.h
 * @brief Qt Vulkan 渲染控件，封装完整的 Vulkan 渲染循环
 * @author hxxcxx
 * @date 2026-04-16
 *
 * 职责：
 * - 管理 VKDevice / VKSwapChain / Shader / PSO 生命周期
 * - UBO 创建与更新（Camera / Object / Material）
 * - 帧循环（beginFrame → renderPass → draw → presentFrame）
 * - 鼠标交互 → Camera
 */

#pragma once

#include <QWidget>
#include <QTimer>

#include <MulanGeo/Engine/RHI/Vulkan/VKDevice.h>
#include <MulanGeo/Engine/RHI/Vulkan/VKSwapChain.h>
#include <MulanGeo/Engine/RHI/Vulkan/VKCommandList.h>
#include <MulanGeo/Engine/RHI/Vulkan/VKShader.h>
#include <MulanGeo/Engine/RHI/Vulkan/VKPipelineState.h>
#include <MulanGeo/Engine/RHI/Vulkan/VKBuffer.h>
#include <MulanGeo/Engine/RHI/Vulkan/VKDescriptorAllocator.h>
#include <MulanGeo/Engine/Scene/Camera.h>
#include <MulanGeo/Engine/Render/SceneRenderer.h>
#include <MulanGeo/Engine/Render/RenderGeometry.h>
#include <MulanGeo/Engine/Render/GeometryCache.h>
#include <MulanGeo/Engine/Window.h>
#include <MulanGeo/Engine/Math/Mat4.h>
#include <MulanGeo/Engine/Math/Vec3.h>
#include <MulanGeo/Engine/RHI/VertexLayout.h>

#include <memory>
#include <vector>
#include <cstdio>

namespace MulanGeo::IO {
struct ImportResult;
struct MeshPart;
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
    float _pad1[3];     // 3x3 补齐到 16 字节对齐
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
// VulkanRenderWidget
// ============================================================

class VulkanRenderWidget : public QWidget {
    Q_OBJECT

public:
    explicit VulkanRenderWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(320, 240);
    }

    ~VulkanRenderWidget() {
        // 等 GPU 空闲再销毁
        if (m_device) m_device->waitIdle();
        cleanup();
    }

    /// 初始化 Vulkan（在 showEvent 中调用）
    bool initVulkan() {
        if (m_initialized) return true;

        // --- Device ---
        Engine::RenderConfig config;
        config.bufferCount   = 2;
        config.vsync         = true;
        config.depthBuffer   = true;
        config.stencilBuffer = false;

        Engine::VKDevice::CreateInfo ci;
        ci.window       = Engine::NativeWindowHandle::makeWin32(
            reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),
            reinterpret_cast<uintptr_t>((HWND)winId()));
        ci.renderConfig   = config;
        ci.enableValidation = true;

        m_device = std::make_unique<Engine::VKDevice>(ci);
        if (!m_device->vkDevice()) {
            qWarning("Failed to create VKDevice");
            return false;
        }

        // --- SwapChain ---
        Engine::SwapChainDesc scDesc;
        scDesc.width  = static_cast<uint32_t>(width());
        scDesc.height = static_cast<uint32_t>(height());
        scDesc.format = Engine::TextureFormat::BGRA8_UNorm;
        scDesc.bufferCount = 2;
        scDesc.vsync = true;

        m_swapchain = static_cast<Engine::VKSwapChain*>(
            m_device->createSwapChain(scDesc));

        // --- Shaders ---
        loadShaders();

        // --- PSO ---
        createPSOs();

        // --- UBO ---
        createUBOs();

        // --- Camera ---
        m_camera.setSize(width(), height());
        m_camera.fitToBox(Engine::AABB(Engine::Vec3(-1, -1, -1), Engine::Vec3(1, 1, 1)));

        // --- Renderer ---
        m_renderer = std::make_unique<Engine::SceneRenderer>(m_device.get());
        m_renderer->setSolidPipeline(m_solidPso);

        m_initialized = true;
        return true;
    }

    /// 加载 mesh 数据
    void loadMesh(const MulanGeo::IO::ImportResult& result);

    /// 手动触发重绘
    void requestFrame() {
        if (m_initialized && isVisible()) {
            renderFrame();
        }
    }

protected:
    void showEvent(QShowEvent* e) override {
        QWidget::showEvent(e);
        if (!m_initialized) {
            if (!initVulkan()) return;
            // 启动渲染定时器
            if (!m_timer) {
                m_timer = new QTimer(this);
                connect(m_timer, &QTimer::timeout, this, [this]() { requestFrame(); });
                m_timer->start(16); // ~60 FPS
            }
        }
    }

    void resizeEvent(QResizeEvent* e) override {
        QWidget::resizeEvent(e);
        if (m_swapchain && m_device) {
            m_device->waitIdle();
            m_swapchain->resize(width(), height());
            // PSO 需要重建（renderPass 变了）
            if (m_solidPso) {
                // SwapChain resize 后 renderPass 不变（我们自己创建的），不需要重建 PSO
            }
            m_camera.setSize(width(), height());
        }
    }

    void paintEvent(QPaintEvent*) override {
        if (m_initialized) requestFrame();
    }

    void mousePressEvent(QMouseEvent* e) override {
        m_lastMousePos = e->pos();
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        int dx = e->pos().x() - m_lastMousePos.x();
        int dy = e->pos().y() - m_lastMousePos.y();

        if (e->buttons() & Qt::LeftButton) {
            m_camera.orbit(dx * 0.005, dy * 0.005);
        } else if (e->buttons() & Qt::RightButton) {
            m_camera.pan(dx * 0.005 * static_cast<float>(m_camera.distance()),
                        -dy * 0.005 * static_cast<float>(m_camera.distance()));
        }
        m_lastMousePos = e->pos();
    }

    void wheelEvent(QWheelEvent* e) override {
        float delta = e->angleDelta().y() / 120.0f;
        m_camera.zoom(delta * 0.1f);
    }

private:
    // --------------------------------------------------------
    // Shader 加载
    // --------------------------------------------------------

    void loadShaders() {
        // 从 build/shaders/ 加载 SPIR-V
        auto loadSPIRV = [](const char* path) -> std::vector<uint8_t> {
            FILE* f = nullptr;
            fopen_s(&f, path, "rb");
            if (!f) {
                qWarning("Failed to open shader: %s", path);
                return {};
            }
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            std::vector<uint8_t> data(size);
            fread(data.data(), 1, size, f);
            fclose(f);
            return data;
        };

#ifdef SHADER_DIR
        std::string shaderDir = SHADER_DIR;
#else
        std::string shaderDir = "shaders";
#endif

        auto solidVsData = loadSPIRV((shaderDir + "/solid.vert.spv").c_str());
        auto solidFsData = loadSPIRV((shaderDir + "/solid.frag.spv").c_str());

        Engine::ShaderDesc vsDesc;
        vsDesc.type = Engine::ShaderType::Vertex;
        vsDesc.byteCode = solidVsData.data();
        vsDesc.byteCodeSize = static_cast<uint32_t>(solidVsData.size());
        m_solidVs = static_cast<Engine::VKShader*>(m_device->createShader(vsDesc));

        Engine::ShaderDesc fsDesc;
        fsDesc.type = Engine::ShaderType::Fragment;
        fsDesc.byteCode = solidFsData.data();
        fsDesc.byteCodeSize = static_cast<uint32_t>(solidFsData.size());
        m_solidFs = static_cast<Engine::VKShader*>(m_device->createShader(fsDesc));
    }

    // --------------------------------------------------------
    // PSO 创建
    // --------------------------------------------------------

    void createPSOs() {
        // CAD 标准顶点布局: pos(3f) + normal(3f) + uv(2f) = 32 bytes
        m_vertexLayout.begin()
            .add(Engine::VertexSemantic::Position, Engine::VertexFormat::RGB32Float)
            .add(Engine::VertexSemantic::Normal,   Engine::VertexFormat::RGB32Float)
            .add(Engine::VertexSemantic::TexCoord,  Engine::VertexFormat::RG32Float);

        Engine::GraphicsPipelineDesc solidDesc;
        solidDesc.name = "Solid";
        solidDesc.vs   = m_solidVs;
        solidDesc.ps   = m_solidFs;
        solidDesc.vertexLayout = m_vertexLayout;
        solidDesc.topology     = Engine::PrimitiveTopology::TriangleList;
        solidDesc.cullMode     = Engine::CullMode::Back;
        solidDesc.frontFace    = Engine::FrontFace::CounterClockwise;
        solidDesc.fillMode     = Engine::FillMode::Solid;
        solidDesc.depthStencil.depthEnable = true;
        solidDesc.depthStencil.depthWrite  = true;
        solidDesc.depthStencil.depthFunc   = Engine::CompareFunc::LessEqual;

        m_solidPso = static_cast<Engine::VKPipelineState*>(
            m_device->createPipelineState(solidDesc));

        // build PSO（需要 renderPass）
        m_solidPso->build(m_swapchain->renderPass());
    }

    // --------------------------------------------------------
    // UBO 创建
    // --------------------------------------------------------

    void createUBOs() {
        // Camera UBO
        m_cameraBuffer = static_cast<Engine::VKBuffer*>(
            m_device->createBuffer(Engine::BufferDesc::uniform(sizeof(CameraUBO), "CameraUBO")));

        // Object UBO（复用，每 draw call 更新）
        m_objectBuffer = static_cast<Engine::VKBuffer*>(
            m_device->createBuffer(Engine::BufferDesc::uniform(sizeof(ObjectUBO), "ObjectUBO")));

        // Material UBO
        m_materialBuffer = static_cast<Engine::VKBuffer*>(
            m_device->createBuffer(Engine::BufferDesc::uniform(sizeof(MaterialUBO), "MaterialUBO")));

        // 初始化 Material
        MaterialUBO mat{};
        mat.baseColor[0] = 0.85f; mat.baseColor[1] = 0.85f; mat.baseColor[2] = 0.88f;
        mat.lightDir[0]  = 0.5f;  mat.lightDir[1]  = 0.8f;  mat.lightDir[2]  = 0.6f;
        mat.ambientColor[0] = 0.15f; mat.ambientColor[1] = 0.15f; mat.ambientColor[2] = 0.15f;
        mat.wireColor[0] = 0.2f; mat.wireColor[1] = 0.2f; mat.wireColor[2] = 0.2f;
        m_materialBuffer->update(0, sizeof(MaterialUBO), &mat);
    }

    // --------------------------------------------------------
    // 渲染帧
    // --------------------------------------------------------

    void renderFrame() {
        auto* vkDevice = m_device.get();

        // 1. beginFrame（等待上一帧、reset）
        vkDevice->beginFrame();

        auto& frameCtx = vkDevice->currentFrameContext();

        // 2. acquireNextImage
        if (!m_swapchain->acquireNextImage(frameCtx.imageAvailable())) {
            return;
        }

        // 3. 更新 Camera UBO
        updateCameraUBO();

        // 4. 录制命令
        auto cmdBuf = frameCtx.cmdBuffer();

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        cmdBuf.begin(beginInfo);

        // beginRenderPass
        vk::ClearValue clearValues[2];
        auto& rc = vkDevice->renderConfig();
        clearValues[0].color = vk::ClearColorValue(
            std::array<float, 4>{rc.clearColor[0], rc.clearColor[1], rc.clearColor[2], rc.clearColor[3]});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(rc.clearDepth, 0);

        vk::RenderPassBeginInfo rpBegin;
        rpBegin.renderPass      = m_swapchain->renderPass();
        rpBegin.framebuffer     = m_swapchain->currentFramebuffer();
        rpBegin.renderArea      = vk::Rect2D({0, 0}, {static_cast<uint32_t>(width()), static_cast<uint32_t>(height())});
        rpBegin.clearValueCount = 2;
        rpBegin.pClearValues    = clearValues;
        cmdBuf.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

        // 绑定 PSO
        if (m_solidPso && m_solidPso->pipeline()) {
            cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_solidPso->pipeline());

            // 分配 descriptor set 并绑定 UBO
            auto& descAlloc = vkDevice->descriptorAllocator();
            vk::DescriptorSet descSet = descAlloc.allocate(m_solidPso->descriptorSetLayout());

            descAlloc.bindUniformBuffer(descSet, 0,
                m_cameraBuffer->vkBuffer(), 0, sizeof(CameraUBO));
            descAlloc.bindUniformBuffer(descSet, 1,
                m_objectBuffer->vkBuffer(), 0, sizeof(ObjectUBO));
            descAlloc.bindUniformBuffer(descSet, 2,
                m_materialBuffer->vkBuffer(), 0, sizeof(MaterialUBO));

            cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                m_solidPso->layout(), 0, 1, &descSet, 0, nullptr);

            // 设置 viewport / scissor
            vk::Viewport viewport;
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = static_cast<float>(width());
            viewport.height   = static_cast<float>(height());
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            cmdBuf.setViewport(0, 1, &viewport);

            vk::Rect2D scissor;
            scissor.offset = vk::Offset2D(0, 0);
            scissor.extent = vk::Extent2D(static_cast<uint32_t>(width()), static_cast<uint32_t>(height()));
            cmdBuf.setScissor(0, 1, &scissor);

            // 绘制所有 mesh
            drawMeshes(cmdBuf);
        }

        cmdBuf.endRenderPass();
        cmdBuf.end();

        // 5. present
        vkDevice->presentFrame(m_swapchain);
    }

    // --------------------------------------------------------
    // 更新 Camera UBO
    // --------------------------------------------------------

    void updateCameraUBO() {
        CameraUBO ubo{};

        // Mat4 → float[16]（column-major，直接 memcpy）
        auto viewProj = m_camera.viewProjectionMatrix();
        auto view     = m_camera.viewMatrix();
        auto proj     = m_camera.projectionMatrix();

        memcpy(ubo.view,           view.data(),     64);
        memcpy(ubo.projection,     proj.data(),     64);
        memcpy(ubo.viewProjection, viewProj.data(), 64);

        auto pos = m_camera.position();
        ubo.cameraPos[0] = static_cast<float>(pos.x);
        ubo.cameraPos[1] = static_cast<float>(pos.y);
        ubo.cameraPos[2] = static_cast<float>(pos.z);

        m_cameraBuffer->update(0, sizeof(CameraUBO), &ubo);
    }

    // --------------------------------------------------------
    // 绘制 mesh
    // --------------------------------------------------------

    void drawMeshes(vk::CommandBuffer cmdBuf) {
        for (auto& mesh : m_gpuMeshes) {
            if (!mesh.vertexBuffer || mesh.indexCount == 0) continue;

            // 更新 Object UBO（identity transform）
            ObjectUBO obj{};
            // identity matrix (column-major)
            obj.world[0] = 1.0f; obj.world[5] = 1.0f; obj.world[10] = 1.0f; obj.world[15] = 1.0f;
            // identity normal matrix
            obj.normalMat[0] = 1.0f; obj.normalMat[4] = 1.0f; obj.normalMat[8] = 1.0f;
            m_objectBuffer->update(0, sizeof(ObjectUBO), &obj);

            // 绑定 VB / IB
            vk::Buffer vb = mesh.vertexBuffer;
            vk::DeviceSize offset = 0;
            cmdBuf.bindVertexBuffers(0, 1, &vb, &offset);

            if (mesh.indexBuffer) {
                cmdBuf.bindIndexBuffer(mesh.indexBuffer, 0, vk::IndexType::eUint32);
                cmdBuf.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
            } else {
                cmdBuf.draw(mesh.vertexCount, 1, 0, 0);
            }
        }
    }

    // --------------------------------------------------------
    // 清理
    // --------------------------------------------------------

    void cleanup() {
        m_gpuMeshes.clear();

        if (m_device) {
            if (m_solidVs)      m_device->destroy(m_solidVs);
            if (m_solidFs)      m_device->destroy(m_solidFs);
            if (m_solidPso)     m_device->destroy(m_solidPso);
            if (m_cameraBuffer) m_device->destroy(m_cameraBuffer);
            if (m_objectBuffer) m_device->destroy(m_objectBuffer);
            if (m_materialBuffer) m_device->destroy(m_materialBuffer);
            // swapchain 由 device 析构时清理
        }
    }

    // --------------------------------------------------------
    // 成员
    // --------------------------------------------------------

    std::unique_ptr<Engine::VKDevice>         m_device;
    Engine::VKSwapChain*                       m_swapchain  = nullptr;

    // Shader
    Engine::VKShader*                          m_solidVs    = nullptr;
    Engine::VKShader*                          m_solidFs    = nullptr;

    // PSO
    Engine::VKPipelineState*                   m_solidPso   = nullptr;
    Engine::VertexLayout                       m_vertexLayout;

    // UBO
    Engine::VKBuffer*                          m_cameraBuffer   = nullptr;
    Engine::VKBuffer*                          m_objectBuffer   = nullptr;
    Engine::VKBuffer*                          m_materialBuffer = nullptr;

    // Camera
    Engine::Camera                             m_camera;
    QPoint                                     m_lastMousePos;

    // Renderer
    std::unique_ptr<Engine::SceneRenderer>     m_renderer;

    // GPU mesh 数据
    struct GpuMesh {
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        uint32_t   vertexCount = 0;
        uint32_t   indexCount  = 0;
    };
    std::vector<GpuMesh> m_gpuMeshes;

    QTimer*  m_timer      = nullptr;
    bool     m_initialized = false;
};
