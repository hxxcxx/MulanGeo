/**
 * @file RenderWidget.cpp
 * @brief 通用渲染控件实现，使用 RHI 接口驱动渲染循环
 * @author hxxcxx
 * @date 2026-04-16
 */

#include "RenderWidget.h"

#include <MulanGeo/IO/IFileImporter.h>
#include <MulanGeo/IO/ImporterFactory.h>
#include <MulanGeo/IO/MeshData.h>

#include <QShowEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace MulanGeo::Engine;

// ============================================================
// 构造 / 析构
// ============================================================

RenderWidget::RenderWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
}

RenderWidget::~RenderWidget() {
    if (m_device) m_device->waitIdle();
    cleanup();
}

// ============================================================
// 初始化
// ============================================================

bool RenderWidget::initRHI() {
    if (m_initialized) return true;

    // --- Device（通过工厂创建）---
    RenderConfig config;
    config.bufferCount   = 2;
    config.vsync         = true;
    config.depthBuffer   = true;
    config.stencilBuffer = false;

    DeviceCreateInfo ci;
    ci.backend          = GraphicsBackend::Vulkan;
#ifdef _WIN32
    ci.window           = NativeWindowHandle::makeWin32(
        reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),
        reinterpret_cast<uintptr_t>(HWND(winId())));
#endif
    ci.renderConfig     = config;
    ci.enableValidation = true;

    m_device = RHIDevice::create(ci);
    if (!m_device) {
        qWarning("Failed to create RHI device");
        return false;
    }

    // --- SwapChain ---
    SwapChainDesc scDesc;
    scDesc.width       = static_cast<uint32_t>(width());
    scDesc.height      = static_cast<uint32_t>(height());
    scDesc.format      = TextureFormat::BGRA8_UNorm;
    scDesc.bufferCount = 2;
    scDesc.vsync       = true;

    m_swapchain = m_device->createSwapChain(scDesc);

    // --- Shaders ---
    loadShaders();

    // --- PSO ---
    createPSOs();

    // --- UBO ---
    createUBOs();

    // --- Camera ---
    m_camera.setSize(width(), height());
    m_camera.fitToBox(AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));

    m_initialized = true;
    return true;
}

// ============================================================
// Shader 加载
// ============================================================

void RenderWidget::loadShaders() {
    auto loadSPIRV = [](const char* path) -> std::vector<uint8_t> {
        FILE* f = nullptr;
#ifdef _WIN32
        fopen_s(&f, path, "rb");
#else
        f = fopen(path, "rb");
#endif
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

    ShaderDesc vsDesc;
    vsDesc.type         = ShaderType::Vertex;
    vsDesc.byteCode     = solidVsData.data();
    vsDesc.byteCodeSize = static_cast<uint32_t>(solidVsData.size());
    m_solidVs = m_device->createShader(vsDesc);

    ShaderDesc fsDesc;
    fsDesc.type         = ShaderType::Fragment;
    fsDesc.byteCode     = solidFsData.data();
    fsDesc.byteCodeSize = static_cast<uint32_t>(solidFsData.size());
    m_solidFs = m_device->createShader(fsDesc);
}

// ============================================================
// PSO 创建
// ============================================================

void RenderWidget::createPSOs() {
    // CAD 标准顶点布局: pos(3f) + normal(3f) + uv(2f) = 32 bytes
    m_vertexLayout.begin()
        .add(VertexSemantic::Position, VertexFormat::RGB32Float)
        .add(VertexSemantic::Normal,   VertexFormat::RGB32Float)
        .add(VertexSemantic::TexCoord, VertexFormat::RG32Float);

    GraphicsPipelineDesc solidDesc;
    solidDesc.name                  = "Solid";
    solidDesc.vs                    = m_solidVs;
    solidDesc.ps                    = m_solidFs;
    solidDesc.vertexLayout          = m_vertexLayout;
    solidDesc.topology              = PrimitiveTopology::TriangleList;
    solidDesc.cullMode              = CullMode::Back;
    solidDesc.frontFace             = FrontFace::CounterClockwise;
    solidDesc.fillMode              = FillMode::Solid;
    solidDesc.depthStencil.depthEnable = true;
    solidDesc.depthStencil.depthWrite  = true;
    solidDesc.depthStencil.depthFunc   = CompareFunc::LessEqual;

    m_solidPso = m_device->createPipelineState(solidDesc);

    // 在 SwapChain 就绪后完成管线构建
    m_solidPso->finalize(m_swapchain);
}

// ============================================================
// UBO 创建
// ============================================================

void RenderWidget::createUBOs() {
    m_cameraBuffer = m_device->createBuffer(
        BufferDesc::uniform(sizeof(CameraUBO), "CameraUBO"));

    m_objectBuffer = m_device->createBuffer(
        BufferDesc::uniform(sizeof(ObjectUBO), "ObjectUBO"));

    m_materialBuffer = m_device->createBuffer(
        BufferDesc::uniform(sizeof(MaterialUBO), "MaterialUBO"));

    // 初始化 Material
    MaterialUBO mat{};
    mat.baseColor[0] = 0.85f; mat.baseColor[1] = 0.85f; mat.baseColor[2] = 0.88f;
    mat.lightDir[0]  = 0.5f;  mat.lightDir[1]  = 0.8f;  mat.lightDir[2]  = 0.6f;
    mat.ambientColor[0] = 0.15f; mat.ambientColor[1] = 0.15f; mat.ambientColor[2] = 0.15f;
    mat.wireColor[0] = 0.2f; mat.wireColor[1] = 0.2f; mat.wireColor[2] = 0.2f;
    m_materialBuffer->update(0, sizeof(MaterialUBO), &mat);
}

// ============================================================
// 帧循环
// ============================================================

void RenderWidget::renderFrame() {
    // 1. beginFrame：等待上一帧完成、acquire next image、重置资源
    m_device->beginFrame();

    auto* cmd = m_device->frameCommandList();

    // 2. 更新 Camera UBO
    updateCameraUBO();

    // 3. 录制命令
    cmd->begin();

    // beginRenderPass（swapchain 自动处理 clear + framebuffer 绑定）
    m_swapchain->beginRenderPass(cmd);

    // 绑定 PSO
    cmd->setPipelineState(m_solidPso);

    // 绑定 UBO（Camera b0, Object b1, Material b2）
    RHIDevice::UniformBufferBind uboBinds[] = {
        { 0, m_cameraBuffer,   0, sizeof(CameraUBO) },
        { 1, m_objectBuffer,   0, sizeof(ObjectUBO) },
        { 2, m_materialBuffer, 0, sizeof(MaterialUBO) },
    };
    m_device->bindUniformBuffers(cmd, m_solidPso, uboBinds, 3);

    // 设置 viewport / scissor
    Viewport vp;
    vp.x       = 0.0f;
    vp.y       = 0.0f;
    vp.width   = static_cast<float>(width());
    vp.height  = static_cast<float>(height());
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    cmd->setViewport(vp);

    ScissorRect sc;
    sc.x      = 0;
    sc.y      = 0;
    sc.width  = static_cast<int32_t>(width());
    sc.height = static_cast<int32_t>(height());
    cmd->setScissorRect(sc);

    // 绘制所有 mesh
    drawMeshes();

    // endRenderPass
    m_swapchain->endRenderPass(cmd);

    cmd->end();

    // 4. 提交 + present
    m_device->submitAndPresent(m_swapchain);
}

// ============================================================
// 更新 Camera UBO
// ============================================================

void RenderWidget::updateCameraUBO() {
    CameraUBO ubo{};

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

// ============================================================
// 绘制 mesh
// ============================================================

void RenderWidget::drawMeshes() {
    auto* cmd = m_device->frameCommandList();

    for (auto& mesh : m_gpuMeshes) {
        if (!mesh.vertexBuffer || mesh.indexCount == 0) continue;

        // 更新 Object UBO（identity transform）
        ObjectUBO obj{};
        obj.world[0] = 1.0f; obj.world[5] = 1.0f;
        obj.world[10] = 1.0f; obj.world[15] = 1.0f;
        obj.normalMat[0] = 1.0f; obj.normalMat[4] = 1.0f; obj.normalMat[8] = 1.0f;
        m_objectBuffer->update(0, sizeof(ObjectUBO), &obj);

        // 绑定 VB / IB
        cmd->setVertexBuffer(0, mesh.vertexBuffer);
        if (mesh.indexBuffer) {
            cmd->setIndexBuffer(mesh.indexBuffer);
            cmd->drawIndexed(DrawIndexedAttribs(mesh.indexCount));
        } else {
            cmd->draw(DrawAttribs(mesh.vertexCount));
        }
    }
}

// ============================================================
// 加载 mesh
// ============================================================

void RenderWidget::loadMesh(const MulanGeo::IO::ImportResult& result) {
    // 清除旧数据
    for (auto& mesh : m_gpuMeshes) {
        if (mesh.vertexBuffer) m_device->destroy(mesh.vertexBuffer);
        if (mesh.indexBuffer)  m_device->destroy(mesh.indexBuffer);
    }
    m_gpuMeshes.clear();

    for (const auto& src : result.meshes) {
        GpuMesh gpu{};

        // 顶点数据: pos(3f) + normal(3f) + uv(2f) = 32 bytes/vertex
        struct VertP3N3UV2 {
            float px, py, pz;
            float nx, ny, nz;
            float u, v;
        };

        std::vector<VertP3N3UV2> vertices(src.vertices.size());
        for (size_t i = 0; i < src.vertices.size(); ++i) {
            vertices[i] = {
                src.vertices[i].x, src.vertices[i].y, src.vertices[i].z,
                src.vertices[i].nx, src.vertices[i].ny, src.vertices[i].nz,
                src.vertices[i].u, src.vertices[i].v
            };
        }

        if (!vertices.empty()) {
            auto vbd = BufferDesc::vertex(
                static_cast<uint32_t>(vertices.size() * sizeof(VertP3N3UV2)),
                sizeof(VertP3N3UV2), "VB");
            gpu.vertexBuffer = m_device->createBuffer(vbd);
            gpu.vertexBuffer->update(0,
                static_cast<uint32_t>(vertices.size() * sizeof(VertP3N3UV2)),
                vertices.data());
            gpu.vertexCount = static_cast<uint32_t>(vertices.size());
        }

        if (!src.indices.empty()) {
            auto ibd = BufferDesc::index(
                static_cast<uint32_t>(src.indices.size() * sizeof(uint32_t)),
                IndexType::UInt32, "IB");
            gpu.indexBuffer = m_device->createBuffer(ibd);
            gpu.indexBuffer->update(0,
                static_cast<uint32_t>(src.indices.size() * sizeof(uint32_t)),
                src.indices.data());
            gpu.indexCount = static_cast<uint32_t>(src.indices.size());
        }

        m_gpuMeshes.push_back(gpu);
    }
}

// ============================================================
// Qt 事件
// ============================================================

void RenderWidget::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    if (!m_initialized) {
        if (!initRHI()) return;
        if (!m_timer) {
            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, [this]() { requestFrame(); });
            m_timer->start(16); // ~60 FPS
        }
    }
}

void RenderWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_swapchain && m_device) {
        m_device->waitIdle();
        m_swapchain->resize(width(), height());
        // PSO 需要 rebuild（renderPass 可能变化）
        if (m_solidPso) {
            m_solidPso->finalize(m_swapchain);
        }
        m_camera.setSize(width(), height());
    }
}

void RenderWidget::paintEvent(QPaintEvent*) {
    if (m_initialized) requestFrame();
}

void RenderWidget::mousePressEvent(QMouseEvent* e) {
    m_lastMousePos = e->pos();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e) {
    int dx = e->pos().x() - m_lastMousePos.x();
    int dy = e->pos().y() - m_lastMousePos.y();

    if (e->buttons() & Qt::LeftButton) {
        m_camera.orbit(dx * 0.005f, dy * 0.005f);
    } else if (e->buttons() & Qt::RightButton) {
        m_camera.pan(dx * 0.005f * static_cast<float>(m_camera.distance()),
                    -dy * 0.005f * static_cast<float>(m_camera.distance()));
    }
    m_lastMousePos = e->pos();
}

void RenderWidget::wheelEvent(QWheelEvent* e) {
    float delta = e->angleDelta().y() / 120.0f;
    m_camera.zoom(delta * 0.1f);
}

// ============================================================
// 清理
// ============================================================

void RenderWidget::cleanup() {
    for (auto& mesh : m_gpuMeshes) {
        if (mesh.vertexBuffer) m_device->destroy(mesh.vertexBuffer);
        if (mesh.indexBuffer)  m_device->destroy(mesh.indexBuffer);
    }
    m_gpuMeshes.clear();

    if (m_device) {
        if (m_solidVs)        m_device->destroy(m_solidVs);
        if (m_solidFs)        m_device->destroy(m_solidFs);
        if (m_solidPso)       m_device->destroy(m_solidPso);
        if (m_cameraBuffer)   m_device->destroy(m_cameraBuffer);
        if (m_objectBuffer)   m_device->destroy(m_objectBuffer);
        if (m_materialBuffer) m_device->destroy(m_materialBuffer);
    }
}

void RenderWidget::requestFrame() {
    if (m_initialized && isVisible()) {
        renderFrame();
    }
}
