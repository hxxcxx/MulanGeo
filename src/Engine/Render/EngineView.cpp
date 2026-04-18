/**
 * @file EngineView.cpp
 * @brief EngineView 实现 — 引擎渲染/交互的核心逻辑
 * @author hxxcxx
 * @date 2026-04-17
 */

#include "EngineView.h"

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
    auto* dev = m_device.get();

    // --- SwapChain ---
    SwapChainDesc scDesc;
    scDesc.width       = static_cast<uint32_t>(width);
    scDesc.height      = static_cast<uint32_t>(height);
    scDesc.format      = TextureFormat::BGRA8_UNorm;
    scDesc.bufferCount = 2;
    scDesc.vsync       = true;

    m_swapchain = makeResource(dev->createSwapChain(scDesc), dev);
    if (!m_swapchain) { cleanup(); return false; }

    // --- Graphics Resources ---
    loadShaders();
    if (!m_solidVs || !m_solidFs) { cleanup(); return false; }

    createPSOs();
    createUBOs();

    // --- Camera ---
    m_camera.setViewport(width, height);
    m_camera.fitToBox(AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));

    // --- Scene Renderer ---
    initSceneRenderer();

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
    auto* dev = m_device.get();

    // --- RenderTarget ---
    RenderTargetDesc rtDesc;
    rtDesc.width       = static_cast<uint32_t>(width);
    rtDesc.height      = static_cast<uint32_t>(height);
    rtDesc.colorFormat = TextureFormat::RGBA8_UNorm;
    rtDesc.depthFormat = TextureFormat::D24_UNorm_S8_UInt;
    rtDesc.hasDepth    = true;

    m_renderTarget = makeResource(dev->createRenderTarget(rtDesc), dev);
    if (!m_renderTarget) { cleanup(); return false; }

    // --- Staging buffer（用于 readback）---
    uint32_t pixelBytes = static_cast<uint32_t>(width) * height * 4;
    m_stagingBuffer = makeResource(dev->createBuffer(
        BufferDesc::staging(pixelBytes, "ReadbackStaging")), dev);

    // --- Graphics Resources ---
    loadShaders();
    if (!m_solidVs || !m_solidFs) { cleanup(); return false; }

    createPSOs();
    createUBOs();

    // --- Camera ---
    m_camera.setViewport(width, height);
    m_camera.fitToBox(AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));

    // --- Scene Renderer ---
    initSceneRenderer();

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

    auto* dev = m_device.get();
    if (m_renderTarget) {
        m_renderTarget->resize(width, height);

        // 重建 staging buffer
        m_stagingBuffer.reset();
        uint32_t pixelBytes = static_cast<uint32_t>(width) * height * 4;
        m_stagingBuffer = makeResource(dev->createBuffer(
            BufferDesc::staging(pixelBytes, "ReadbackStaging")), dev);

        if (m_solidPso) m_solidPso->finalize(m_renderTarget.get());
    } else {
        m_swapchain->resize(width, height);
        if (m_solidPso) m_solidPso->finalize(m_swapchain.get());
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
    m_solidVs = makeResource(m_device->createShader(vsDesc), m_device.get());

    ShaderDesc fsDesc;
    fsDesc.type         = ShaderType::Pixel;
    fsDesc.byteCode     = solidFsData.data();
    fsDesc.byteCodeSize = static_cast<uint32_t>(solidFsData.size());
    m_solidFs = makeResource(m_device->createShader(fsDesc), m_device.get());
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
    solidDesc.vs                    = m_solidVs.get();
    solidDesc.ps                    = m_solidFs.get();
    solidDesc.vertexLayout          = m_vertexLayout;
    solidDesc.topology              = PrimitiveTopology::TriangleList;
    solidDesc.cullMode              = CullMode::Back;
    solidDesc.frontFace             = FrontFace::CounterClockwise;
    solidDesc.fillMode              = FillMode::Solid;
    solidDesc.depthStencil.depthEnable = true;
    solidDesc.depthStencil.depthWrite  = true;
    solidDesc.depthStencil.depthFunc   = CompareFunc::LessEqual;

    // Descriptor bindings: set 0 = Camera, 1 = Object, 2 = Material
    using DB = DescriptorBinding;
    solidDesc.descriptorBindings[0] = {0, 1, DB::kStageVertex | DB::kStageFragment};
    solidDesc.descriptorBindings[1] = {1, 1, DB::kStageVertex | DB::kStageFragment};
    solidDesc.descriptorBindings[2] = {2, 1, DB::kStageFragment};
    solidDesc.descriptorBindingCount = 3;

    m_solidPso = makeResource(m_device->createPipelineState(solidDesc), m_device.get());
    if (m_renderTarget) {
        m_solidPso->finalize(m_renderTarget.get());
    } else {
        m_solidPso->finalize(m_swapchain.get());
    }
}

// ============================================================
// UBO
// ============================================================

void EngineView::createUBOs() {
    auto* dev = m_device.get();
    m_cameraBuffer = makeResource(dev->createBuffer(
        BufferDesc::uniform(sizeof(CameraUBO), "CameraUBO")), dev);

    m_objectBuffer = makeResource(dev->createBuffer(
        BufferDesc::uniform(sizeof(ObjectUBO), "ObjectUBO")), dev);

    m_materialBuffer = makeResource(dev->createBuffer(
        BufferDesc::uniform(sizeof(LightMaterialUBO), "MaterialUBO")), dev);

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

    cmd->setPipelineState(m_solidPso.get());

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

    // 委托给 SceneRenderer 绘制
    m_sceneRenderer->render(m_renderQueue, m_camera, cmd);

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
        m_device->submitAndPresent(m_swapchain.get());
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
    cmd->copyTextureToBuffer(m_renderTarget->colorTexture(), m_stagingBuffer.get());
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
// Scene Renderer 初始化
// ============================================================

void EngineView::initSceneRenderer() {
    m_sceneRenderer = std::make_unique<SceneRenderer>(m_device.get());
    m_sceneRenderer->setSolidPipeline(m_solidPso.get());
    m_sceneRenderer->setCameraBuffer(m_cameraBuffer.get());
    m_sceneRenderer->setObjectBuffer(m_objectBuffer.get());
    m_sceneRenderer->setMaterialBuffer(m_materialBuffer.get());
}

// ============================================================
// Camera UBO
// ============================================================

void EngineView::updateCameraUBO() {
    CameraUBO ubo{};

    auto view     = m_camera.viewMatrix();
    auto proj     = m_camera.projectionMatrix();

    auto eye = m_camera.eyePosition();
    fprintf(stderr, "[DEBUG] camera: eye=[%.2f,%.2f,%.2f] dist=%.2f fov=%.2f near=%.4f far=%.1f\n",
            eye.x, eye.y, eye.z, m_camera.distance(), m_camera.fieldOfView(),
            0.01, 10000.0);
    fflush(stderr);

    // 应用后端裁剪空间修正（Vulkan: Y↓ z∈[0,1]，OpenGL: 无修正）
    auto clip     = m_device->clipSpaceCorrectionMatrix();
    auto corrProj = clip * proj;
    auto viewProj = corrProj * view;

    // Mat4 内部是 double，UBO 需要 float，逐元素转换
    auto storeMat4 = [](float* dst, const Mat4& src) {
        for (int i = 0; i < 16; ++i)
            dst[i] = static_cast<float>(src.data()[i]);
    };

    storeMat4(ubo.view,           view);
    storeMat4(ubo.projection,     corrProj);
    storeMat4(ubo.viewProjection, viewProj);

    auto pos = m_camera.eyePosition();
    ubo.cameraPos[0] = static_cast<float>(pos.x);
    ubo.cameraPos[1] = static_cast<float>(pos.y);
    ubo.cameraPos[2] = static_cast<float>(pos.z);

    m_cameraBuffer->update(0, sizeof(CameraUBO), &ubo);
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

void EngineView::loadMesh(const LoadMeshData& data) {
    // 清除旧数据
    m_geometries.clear();
    m_renderQueue.clear();
    m_meshVertexData.clear();
    m_meshIndexData.clear();
    m_sceneRenderer->clearCache();

    const size_t partCount = data.parts.size();
    m_geometries.reserve(partCount);
    m_meshVertexData.reserve(partCount);
    m_meshIndexData.reserve(partCount);

    // 计算模型包围盒
    AABB modelBounds;

    // 第一遍：构建全部 geometry（避免 vector 重分配导致指针失效）
    for (const auto& src : data.parts) {
        RenderGeometry geo{};
        geo.vertexCount  = static_cast<uint32_t>(src.vertices.size() / 8); // P3N3UV2 = 8 floats
        geo.indexCount   = static_cast<uint32_t>(src.indices.size());
        geo.vertexStride = sizeof(float) * 8;
        geo.topology     = PrimitiveTopology::TriangleList;

        // 持有顶点数据
        auto vertexBytes = std::make_shared<std::vector<float>>(src.vertices);
        auto indexBytes  = std::make_shared<std::vector<uint32_t>>(src.indices);

        m_meshVertexData.push_back(vertexBytes);
        m_meshIndexData.push_back(indexBytes);

        geo.vertexBytes = std::as_bytes(std::span{*vertexBytes});
        geo.indexBytes  = std::as_bytes(std::span{*indexBytes});

        m_geometries.push_back(geo);

        // 遍历顶点累积包围盒（P3N3UV2 布局，每8个float取前3个）
        for (uint32_t i = 0; i < geo.vertexCount; ++i) {
            const float* v = src.vertices.data() + i * 8;
            modelBounds.expand(Vec3(v[0], v[1], v[2]));
        }
    }

    // 第二遍：构建 RenderItem（此时 m_geometries 不再重分配）
    for (size_t i = 0; i < m_geometries.size(); ++i) {
        RenderItem item;
        item.geometry = &m_geometries[i];
        item.worldTransform = Mat4::identity();
        item.pickId = static_cast<uint32_t>(i);
        m_renderQueue.add(item);
    }

    // 适配相机到模型
    if (!modelBounds.isEmpty()) {
        m_camera.fitToBox(modelBounds);
    }

    fprintf(stderr, "[DEBUG] loadMesh: %zu geometries, %zu renderItems, bounds=[%.2f,%.2f,%.2f]->[%.2f,%.2f,%.2f]\n",
            m_geometries.size(), m_renderQueue.items().size(),
            modelBounds.min.x, modelBounds.min.y, modelBounds.min.z,
            modelBounds.max.x, modelBounds.max.y, modelBounds.max.z);
    fflush(stderr);
}

// ============================================================
// 清理
// ============================================================

void EngineView::cleanup() {
    m_geometries.clear();
    m_renderQueue.clear();
    m_meshVertexData.clear();
    m_meshIndexData.clear();
    m_sceneRenderer.reset();

    // RAII 资源自动销毁（reset 按逆序）
    m_materialBuffer.reset();
    m_objectBuffer.reset();
    m_cameraBuffer.reset();
    m_solidPso.reset();
    m_solidFs.reset();
    m_solidVs.reset();
    m_stagingBuffer.reset();
    m_renderTarget.reset();
    m_swapchain.reset();
}

void EngineView::shutdown() {
    if (!m_initialized) return;
    if (m_device) m_device->waitIdle();
    cleanup();
    m_initialized = false;
}

} // namespace MulanGeo::Engine
