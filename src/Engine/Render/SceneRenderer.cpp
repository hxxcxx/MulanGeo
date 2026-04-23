/**
 * @file SceneRenderer.cpp
 * @brief SceneRenderer 实现 — 管理管线资源，录制 RHI 绘制命令
 * @author hxxcxx
 * @date 2026-04-15
 */
#include "SceneRenderer.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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

struct alignas(16) MaterialUBO {
    float baseColor[3];    float _p0;
    float lightDir[3];     float _p1;
    float ambientColor[3]; float _p2;
    float wireColor[3];    float _p3;
};

// ============================================================
// 构造
// ============================================================

SceneRenderer::SceneRenderer(RHIDevice* device)
    : m_device(device), m_cache(device) {}

// ============================================================
// 初始化
// ============================================================

bool SceneRenderer::init() {
    loadShaders();
    if (!m_solidVs || !m_solidFs) return false;

    createPSOs();
    createUBOs();
    return true;
}

void SceneRenderer::cleanup() {
    m_cache.clear();
    m_materialBuffer.reset();
    m_objectBuffer.reset();
    m_cameraBuffer.reset();
    m_solidPso.reset();
    m_solidFs.reset();
    m_solidVs.reset();
}

void SceneRenderer::finalizePipeline(SwapChain* swapchain) {
    if (m_solidPso && swapchain)
        m_solidPso->finalize(swapchain);
}

void SceneRenderer::finalizePipeline(RenderTarget* rt) {
    if (m_solidPso && rt)
        m_solidPso->finalize(rt);
}

// ============================================================
// Shader
// ============================================================

void SceneRenderer::loadShaders() {
    auto loadFile = [](const char* path) -> std::vector<uint8_t> {
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

    const char* ext = ".spv";
    if (m_device->backend() == GraphicsBackend::D3D12)
        ext = ".dxil";
    else if (m_device->backend() == GraphicsBackend::D3D11)
        ext = ".dxbc";

    auto vsPath = shaderDir + "/solid.vert" + ext;
    auto fsPath = shaderDir + "/solid.frag" + ext;
    auto solidVsData = loadFile(vsPath.c_str());
    auto solidFsData = loadFile(fsPath.c_str());

    ShaderDesc vsDesc;
    vsDesc.type         = ShaderType::Vertex;
    vsDesc.byteCode     = solidVsData.data();
    vsDesc.byteCodeSize = static_cast<uint32_t>(solidVsData.size());
    m_solidVs = makeResource(m_device->createShader(vsDesc), m_device);

    ShaderDesc fsDesc;
    fsDesc.type         = ShaderType::Pixel;
    fsDesc.byteCode     = solidFsData.data();
    fsDesc.byteCodeSize = static_cast<uint32_t>(solidFsData.size());
    m_solidFs = makeResource(m_device->createShader(fsDesc), m_device);
}

// ============================================================
// PSO
// ============================================================

void SceneRenderer::createPSOs() {
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
    solidDesc.cullMode              = CullMode::None;
    solidDesc.frontFace             = FrontFace::CounterClockwise;
    solidDesc.fillMode              = FillMode::Solid;
    solidDesc.depthStencil.depthEnable = true;
    solidDesc.depthStencil.depthWrite  = true;
    solidDesc.depthStencil.depthFunc   = CompareFunc::LessEqual;

    using DB = DescriptorBinding;
    solidDesc.descriptorBindings[0] = {0, 1, DB::kStageVertex | DB::kStageFragment};
    solidDesc.descriptorBindings[1] = {1, 1, DB::kStageVertex | DB::kStageFragment};
    solidDesc.descriptorBindings[2] = {2, 1, DB::kStageFragment};
    solidDesc.descriptorBindingCount = 3;

    m_solidPso = makeResource(m_device->createPipelineState(solidDesc), m_device);
}

void SceneRenderer::createUBOs() {
    auto* dev = m_device;
    m_cameraBuffer = makeResource(dev->createBuffer(
        BufferDesc::uniform(sizeof(CameraUBO), "CameraUBO")), dev);

    m_objectBuffer = makeResource(dev->createBuffer(
        BufferDesc::uniform(sizeof(ObjectUBO), "ObjectUBO")), dev);

    m_materialBuffer = makeResource(dev->createBuffer(
        BufferDesc::uniform(sizeof(MaterialUBO), "MaterialUBO")), dev);

    MaterialUBO mat{};
    mat.baseColor[0] = 0.85f; mat.baseColor[1] = 0.85f; mat.baseColor[2] = 0.88f;
    mat.lightDir[0]  = 0.5f;  mat.lightDir[1]  = 0.8f;  mat.lightDir[2]  = 0.6f;
    mat.ambientColor[0] = 0.15f; mat.ambientColor[1] = 0.15f; mat.ambientColor[2] = 0.15f;
    mat.wireColor[0] = 0.2f; mat.wireColor[1] = 0.2f; mat.wireColor[2] = 0.2f;
    m_materialBuffer->update(0, sizeof(MaterialUBO), &mat);
}

// ============================================================
// Camera UBO 更新
// ============================================================

void SceneRenderer::updateCameraUBO(const Camera& camera) {
    if (!m_cameraBuffer) return;

    CameraUBO ubo{};

    auto view     = camera.viewMatrix();
    auto proj     = camera.projectionMatrix();
    auto clip     = m_device->clipSpaceCorrectionMatrix();
    auto corrProj = clip * proj;
    auto viewProj = corrProj * view;

    auto storeMat4 = [](float* dst, const Mat4& src) {
        const double* p = glm::value_ptr(src);
        for (int i = 0; i < 16; ++i)
            dst[i] = static_cast<float>(p[i]);
    };

    storeMat4(ubo.view,           view);
    storeMat4(ubo.projection,     corrProj);
    storeMat4(ubo.viewProjection, viewProj);

    auto pos = camera.eyePosition();
    ubo.cameraPos[0] = static_cast<float>(pos.x);
    ubo.cameraPos[1] = static_cast<float>(pos.y);
    ubo.cameraPos[2] = static_cast<float>(pos.z);

    m_cameraBuffer->update(0, sizeof(CameraUBO), &ubo);
}

// ============================================================
// PSO finalize（需要在 swapchain/rendertarget 就绪后调用）
// ============================================================

// ============================================================
// 渲染
// ============================================================

void SceneRenderer::render(const RenderQueue& queue, const Camera& camera, CommandList* cmdList) {
    m_stats = {};

    if (!m_solidPso) return;

    // 更新 Camera UBO（每帧一次）
    updateCameraUBO(camera);

    PipelineState* pso = selectPipeline();
    if (!pso) return;
    cmdList->setPipelineState(pso);

    Viewport vp{0.0f, 0.0f,
                 static_cast<float>(camera.width()),
                 static_cast<float>(camera.height()),
                 0.0f, 1.0f};
    cmdList->setViewport(vp);

    for (const auto& item : queue.items()) {
        drawItem(item, cmdList);
    }
}

PipelineState* SceneRenderer::selectPipeline() const {
    return m_solidPso.get();
}

void SceneRenderer::drawItem(const RenderItem& item, CommandList* cmdList) {
    if (!item.geometry) return;

    const auto* geo = item.geometry;
    const auto* gpu = m_cache.getOrUpload(geo);
    if (!gpu || !gpu->vertexBuffer) return;

    // 更新 Object UBO
    if (m_objectBuffer && m_solidPso) {
        ObjectUBO obj{};
        for (int i = 0; i < 16; ++i)
            obj.world[i] = static_cast<float>(glm::value_ptr(item.worldTransform)[i]);

        Mat3 normalMat3 = glm::transpose(glm::inverse(Mat3(item.worldTransform)));
        for (int col = 0; col < 3; ++col)
            for (int row = 0; row < 3; ++row)
                obj.normalMat[col * 3 + row] = static_cast<float>(normalMat3[col][row]);

        obj.pickId = item.pickId;
        m_objectBuffer->update(0, sizeof(ObjectUBO), &obj);

        // 选中面：覆盖材质为高亮色
        if (item.selected && m_materialBuffer) {
            MaterialUBO hl{};
            hl.baseColor[0] = 0.3f; hl.baseColor[1] = 0.6f; hl.baseColor[2] = 1.0f;
            hl.lightDir[0]  = -0.3f; hl.lightDir[1]  = -1.0f; hl.lightDir[2]  = -0.4f;
            hl.ambientColor[0] = 0.2f; hl.ambientColor[1] = 0.2f; hl.ambientColor[2] = 0.2f;
            hl.wireColor[0] = 0.3f; hl.wireColor[1] = 0.6f; hl.wireColor[2] = 1.0f;
            m_materialBuffer->update(0, sizeof(MaterialUBO), &hl);
        }

        RHIDevice::UniformBufferBind uboBinds[] = {
            { 0, m_cameraBuffer.get(),   0, m_cameraBuffer->desc().size },
            { 1, m_objectBuffer.get(),   0, m_objectBuffer->desc().size },
            { 2, m_materialBuffer.get(), 0, m_materialBuffer->desc().size },
        };
        m_device->bindUniformBuffers(cmdList, m_solidPso.get(), uboBinds, 3);
    }

    cmdList->setVertexBuffer(0, gpu->vertexBuffer);

    if (gpu->indexBuffer && gpu->indexCount > 0) {
        cmdList->setIndexBuffer(gpu->indexBuffer);
        cmdList->drawIndexed(DrawIndexedAttribs{
            .indexCount = gpu->indexCount,
        });
        m_stats.triangles += gpu->indexCount / 3;
    } else {
        cmdList->draw(DrawAttribs{
            .vertexCount = gpu->vertexCount,
        });
        m_stats.triangles += gpu->vertexCount / 3;
    }

    ++m_stats.drawCalls;
    ++m_stats.items;
}

} // namespace MulanGeo::Engine
