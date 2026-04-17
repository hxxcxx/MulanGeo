/**
 * @file EngineView.cpp
 * @brief EngineView 实现 — 引擎渲染/交互的核心逻辑
 * @author hxxcxx
 * @date 2026-04-17
 */

#include "EngineView.h"

#include <MulanGeo/IO/MeshData.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace MulanGeo::Engine {

// ============================================================
// 构造 / 析构
// ============================================================

EngineView::EngineView() {
    // 默认操作器：CameraManipulator
    m_operator = std::make_unique<CameraManipulator>();
}

EngineView::~EngineView() {
    shutdown();
}

// ============================================================
// 初始化
// ============================================================

bool EngineView::init(const NativeWindowHandle& window, int width, int height) {
    if (m_initialized) return true;

    m_width  = width;
    m_height = height;

    // --- Device ---
    RenderConfig config;
    config.bufferCount   = 2;
    config.vsync         = true;
    config.depthBuffer   = true;
    config.stencilBuffer = false;

    DeviceCreateInfo ci;
    ci.backend          = GraphicsBackend::Vulkan;
    ci.window           = window;
    ci.renderConfig     = config;
    ci.enableValidation = true;

    m_device = RHIDevice::create(ci);
    if (!m_device) return false;

    // --- SwapChain ---
    SwapChainDesc scDesc;
    scDesc.width       = static_cast<uint32_t>(width);
    scDesc.height      = static_cast<uint32_t>(height);
    scDesc.format      = TextureFormat::BGRA8_UNorm;
    scDesc.bufferCount = 2;
    scDesc.vsync       = true;

    m_swapchain = m_device->createSwapChain(scDesc);
    if (!m_swapchain) { cleanup(); return false; }

    // --- Graphics Resources ---
    loadShaders();
    if (!m_solidVs || !m_solidFs) { cleanup(); return false; }

    createPSOs();
    createUBOs();

    // --- Camera ---
    m_camera.setViewport(width, height);
    m_camera.fitToBox(AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));

    m_initialized = true;
    return true;
}

// ============================================================
// 离屏初始化
// ============================================================

bool EngineView::initOffscreen(int width, int height) {
    if (m_initialized) return true;

    m_width  = width;
    m_height = height;

    // --- Device（无窗口）---
    RenderConfig config;
    config.bufferCount   = 2;
    config.vsync         = false;
    config.depthBuffer   = true;
    config.stencilBuffer = false;

    DeviceCreateInfo ci;
    ci.backend          = GraphicsBackend::Vulkan;
    ci.window           = {};  // 空窗口 — headless
    ci.renderConfig     = config;
    ci.enableValidation = true;

    m_device = RHIDevice::create(ci);
    if (!m_device) return false;

    // --- RenderTarget ---
    RenderTargetDesc rtDesc;
    rtDesc.width       = static_cast<uint32_t>(width);
    rtDesc.height      = static_cast<uint32_t>(height);
    rtDesc.colorFormat = TextureFormat::RGBA8_UNorm;
    rtDesc.depthFormat = TextureFormat::D24_UNorm_S8_UInt;
    rtDesc.hasDepth    = true;

    m_renderTarget = m_device->createRenderTarget(rtDesc);
    if (!m_renderTarget) { cleanup(); return false; }

    // --- Staging buffer（用于 readback）---
    uint32_t pixelBytes = static_cast<uint32_t>(width) * height * 4;
    m_stagingBuffer = m_device->createBuffer(
        BufferDesc::staging(pixelBytes, "ReadbackStaging"));

    // --- Graphics Resources ---
    loadShaders();
    if (!m_solidVs || !m_solidFs) { cleanup(); return false; }

    createPSOs();
    createUBOs();

    // --- Camera ---
    m_camera.setViewport(width, height);
    m_camera.fitToBox(AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));

    m_initialized = true;
    return true;
}

// ============================================================
// Resize
// ============================================================

void EngineView::resize(int width, int height) {
    if (!m_initialized) return;
    m_width  = width;
    m_height = height;

    m_device->waitIdle();

    if (m_renderTarget) {
        m_renderTarget->resize(width, height);

        // 重建 staging buffer
        if (m_stagingBuffer) m_device->destroy(m_stagingBuffer);
        uint32_t pixelBytes = static_cast<uint32_t>(width) * height * 4;
        m_stagingBuffer = m_device->createBuffer(
            BufferDesc::staging(pixelBytes, "ReadbackStaging"));

        if (m_solidPso) m_solidPso->finalize(m_renderTarget);
    } else {
        m_swapchain->resize(width, height);
        if (m_solidPso) m_solidPso->finalize(m_swapchain);
    }
    m_camera.setViewport(width, height);
}

// ============================================================
// Shader
// ============================================================

void EngineView::loadShaders() {
    auto loadSPIRV = [](const char* path) -> std::vector<uint8_t> {
        FILE* f = nullptr;
#ifdef _WIN32
        fopen_s(&f, path, "rb");
#else
        f = fopen(path, "rb");
#endif
        if (!f) return {};
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
    fsDesc.type         = ShaderType::Pixel;
    fsDesc.byteCode     = solidFsData.data();
    fsDesc.byteCodeSize = static_cast<uint32_t>(solidFsData.size());
    m_solidFs = m_device->createShader(fsDesc);
}

// ============================================================
// PSO
// ============================================================

void EngineView::createPSOs() {
    m_vertexLayout.begin()
        .add(VertexSemantic::Position,  VertexFormat::Float3)
        .add(VertexSemantic::Normal,    VertexFormat::Float3)
        .add(VertexSemantic::TexCoord0, VertexFormat::Float2);

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
    if (m_renderTarget) {
        m_solidPso->finalize(m_renderTarget);
    } else {
        m_solidPso->finalize(m_swapchain);
    }
}

// ============================================================
// UBO
// ============================================================

void EngineView::createUBOs() {
    m_cameraBuffer = m_device->createBuffer(
        BufferDesc::uniform(sizeof(CameraUBO), "CameraUBO"));

    m_objectBuffer = m_device->createBuffer(
        BufferDesc::uniform(sizeof(ObjectUBO), "ObjectUBO"));

    m_materialBuffer = m_device->createBuffer(
        BufferDesc::uniform(sizeof(LightMaterialUBO), "MaterialUBO"));

    LightMaterialUBO mat{};
    mat.baseColor[0] = 0.85f; mat.baseColor[1] = 0.85f; mat.baseColor[2] = 0.88f;
    mat.lightDir[0]  = 0.5f;  mat.lightDir[1]  = 0.8f;  mat.lightDir[2]  = 0.6f;
    mat.ambientColor[0] = 0.15f; mat.ambientColor[1] = 0.15f; mat.ambientColor[2] = 0.15f;
    mat.wireColor[0] = 0.2f; mat.wireColor[1] = 0.2f; mat.wireColor[2] = 0.2f;
    m_materialBuffer->update(0, sizeof(LightMaterialUBO), &mat);
}

// ============================================================
// 帧循环
// ============================================================

void EngineView::renderFrame() {
    if (!m_initialized) return;

    m_device->beginFrame();
    auto* cmd = m_device->frameCommandList();

    updateCameraUBO();

    cmd->begin();

    // --- begin render pass ---
    if (m_renderTarget) {
        m_renderTarget->beginRenderPass(cmd);
    } else {
        m_swapchain->beginRenderPass(cmd);
    }

    cmd->setPipelineState(m_solidPso);

    RHIDevice::UniformBufferBind uboBinds[] = {
        { 0, m_cameraBuffer,   0, sizeof(CameraUBO) },
        { 1, m_objectBuffer,   0, sizeof(ObjectUBO) },
        { 2, m_materialBuffer, 0, sizeof(LightMaterialUBO) },
    };
    m_device->bindUniformBuffers(cmd, m_solidPso, uboBinds, 3);

    Viewport vp;
    vp.x       = 0.0f;
    vp.y       = 0.0f;
    vp.width   = static_cast<float>(m_width);
    vp.height  = static_cast<float>(m_height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    cmd->setViewport(vp);

    ScissorRect sc;
    sc.x      = 0;
    sc.y      = 0;
    sc.width  = static_cast<int32_t>(m_width);
    sc.height = static_cast<int32_t>(m_height);
    cmd->setScissorRect(sc);

    drawMeshes();

    // --- end render pass ---
    if (m_renderTarget) {
        m_renderTarget->endRenderPass(cmd);
    } else {
        m_swapchain->endRenderPass(cmd);
    }

    cmd->end();

    // --- submit ---
    if (m_renderTarget) {
        m_device->submitOffscreen();
    } else {
        m_device->submitAndPresent(m_swapchain);
    }
}

// ============================================================
// 离屏回读
// ============================================================

bool EngineView::readbackPixels(std::vector<uint8_t>& pixels) {
    if (!m_renderTarget || !m_stagingBuffer) return false;

    m_device->waitIdle();

    // 录制 transition + copy 命令
    auto* cmd = m_device->createCommandList();
    cmd->begin();
    cmd->transitionResource(m_renderTarget->colorTexture(), ResourceState::CopySrc);
    cmd->copyTextureToBuffer(m_renderTarget->colorTexture(), m_stagingBuffer);
    cmd->end();

    m_device->executeCommandList(cmd);
    m_device->waitIdle();
    m_device->destroy(cmd);

    // 回读
    uint32_t byteSize = static_cast<uint32_t>(m_width) * m_height * 4;
    pixels.resize(byteSize);
    return m_stagingBuffer->readback(0, byteSize, pixels.data());
}

// ============================================================
// Camera UBO
// ============================================================

void EngineView::updateCameraUBO() {
    CameraUBO ubo{};

    auto viewProj = m_camera.viewProjectionMatrix();
    auto view     = m_camera.viewMatrix();
    auto proj     = m_camera.projectionMatrix();

    memcpy(ubo.view,           view.data(),     64);
    memcpy(ubo.projection,     proj.data(),     64);
    memcpy(ubo.viewProjection, viewProj.data(), 64);

    auto pos = m_camera.eyePosition();
    ubo.cameraPos[0] = static_cast<float>(pos.x);
    ubo.cameraPos[1] = static_cast<float>(pos.y);
    ubo.cameraPos[2] = static_cast<float>(pos.z);

    m_cameraBuffer->update(0, sizeof(CameraUBO), &ubo);
}

// ============================================================
// 绘制
// ============================================================

void EngineView::drawMeshes() {
    auto* cmd = m_device->frameCommandList();

    for (auto& mesh : m_gpuMeshes) {
        if (!mesh.vertexBuffer || mesh.indexCount == 0) continue;

        ObjectUBO obj{};
        obj.world[0] = 1.0f; obj.world[5] = 1.0f;
        obj.world[10] = 1.0f; obj.world[15] = 1.0f;
        obj.normalMat[0] = 1.0f; obj.normalMat[4] = 1.0f; obj.normalMat[8] = 1.0f;
        m_objectBuffer->update(0, sizeof(ObjectUBO), &obj);

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
// 输入处理
// ============================================================

void EngineView::handleInput(const InputEvent& event) {
    if (m_operator) {
        m_operator->handleEvent(event, m_camera);
    }
}

// ============================================================
// Operator 管理
// ============================================================

void EngineView::setOperator(std::unique_ptr<Operator> op) {
    if (m_operator) {
        m_operator->onDeactivate(m_camera);
    }
    m_operator = op ? std::move(op) : std::make_unique<CameraManipulator>();
    m_operator->onActivate(m_camera);
}

// ============================================================
// Mesh 加载
// ============================================================

void EngineView::loadMesh(const MulanGeo::IO::ImportResult& result) {
    // 清除旧 GPU 数据
    for (auto& mesh : m_gpuMeshes) {
        if (mesh.vertexBuffer) m_device->destroy(mesh.vertexBuffer);
        if (mesh.indexBuffer)  m_device->destroy(mesh.indexBuffer);
    }
    m_gpuMeshes.clear();

    for (const auto& src : result.meshes) {
        GpuMesh gpu{};

        struct VertP3N3UV2 {
            float px, py, pz;
            float nx, ny, nz;
            float u, v;
        };

        std::vector<VertP3N3UV2> vertices(src.vertices.size());
        for (size_t i = 0; i < src.vertices.size(); ++i) {
            vertices[i] = {
                src.vertices[i].position.x, src.vertices[i].position.y, src.vertices[i].position.z,
                src.vertices[i].normal.x,   src.vertices[i].normal.y,   src.vertices[i].normal.z,
                src.vertices[i].texCoord.u,  src.vertices[i].texCoord.v
            };
        }

        if (!vertices.empty()) {
            uint32_t vbSize = static_cast<uint32_t>(vertices.size() * sizeof(VertP3N3UV2));
            auto vbd = BufferDesc::vertex(vbSize, vertices.data(), "VB");
            gpu.vertexBuffer = m_device->createBuffer(vbd);
            gpu.vertexCount  = static_cast<uint32_t>(vertices.size());
        }

        if (!src.indices.empty()) {
            uint32_t ibSize = static_cast<uint32_t>(src.indices.size() * sizeof(uint32_t));
            auto ibd = BufferDesc::index(ibSize, src.indices.data(), "IB");
            gpu.indexBuffer = m_device->createBuffer(ibd);
            gpu.indexCount  = static_cast<uint32_t>(src.indices.size());
        }

        m_gpuMeshes.push_back(gpu);
    }
}

// ============================================================
// 清理
// ============================================================

void EngineView::cleanup() {
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
        if (m_stagingBuffer)  m_device->destroy(m_stagingBuffer);
        if (m_renderTarget)   m_device->destroy(m_renderTarget);
        if (m_swapchain)      m_device->destroy(m_swapchain);
    }

    m_solidVs       = nullptr;
    m_solidFs       = nullptr;
    m_solidPso      = nullptr;
    m_cameraBuffer  = nullptr;
    m_objectBuffer  = nullptr;
    m_materialBuffer = nullptr;
    m_stagingBuffer = nullptr;
    m_renderTarget  = nullptr;
    m_swapchain     = nullptr;
}

void EngineView::shutdown() {
    if (!m_initialized) return;
    if (m_device) m_device->waitIdle();
    cleanup();
    m_initialized = false;
}

} // namespace MulanGeo::Engine
