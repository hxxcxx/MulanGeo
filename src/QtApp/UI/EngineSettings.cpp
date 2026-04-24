/**
 * @file EngineSettings.cpp
 * @brief 引擎渲染设置单例实现
 * @author hxxcxx
 * @date 2026-04-24
 */
#include "EngineSettings.h"

using namespace MulanGeo::Engine;

EngineSettings& EngineSettings::instance() {
    static EngineSettings s;
    return s;
}

EngineSettings::EngineSettings() {
    load();
}

// --- 后端 ---

GraphicsBackend EngineSettings::backend() const { return m_backend; }

void EngineSettings::setBackend(GraphicsBackend b) {
    if (m_backend != b) { m_backend = b; save(); }
}

// --- 抗锯齿 ---

RenderConfig::MSAALevel EngineSettings::msaa() const { return m_msaa; }

void EngineSettings::setMsaa(RenderConfig::MSAALevel level) {
    if (m_msaa != level) { m_msaa = level; save(); }
}

// --- VSync ---

bool EngineSettings::vsync() const { return m_vsync; }

void EngineSettings::setVsync(bool v) {
    if (m_vsync != v) { m_vsync = v; save(); }
}

// --- 批量应用 / 读取 ---

void EngineSettings::applyTo(ViewConfig& cfg) const {
    cfg.backend = m_backend;
    cfg.msaa    = m_msaa;
    cfg.vsync   = m_vsync;
}

void EngineSettings::loadFrom(const ViewConfig& cfg) {
    m_backend = cfg.backend;
    m_msaa    = cfg.msaa;
    m_vsync   = cfg.vsync;
    save();
}

// --- 持久化 ---

static int backendToInt(GraphicsBackend b) {
    return static_cast<int>(b);
}
static GraphicsBackend intToBackend(int v) {
    switch (v) {
    case 0: return GraphicsBackend::OpenGL;
    case 1: return GraphicsBackend::Vulkan;
    case 2: return GraphicsBackend::D3D12;
    case 3: return GraphicsBackend::D3D11;
    default: return GraphicsBackend::Vulkan;
    }
}

static int msaaToInt(RenderConfig::MSAALevel l) {
    return static_cast<int>(l);
}
static RenderConfig::MSAALevel intToMsaa(int v) {
    switch (v) {
    case 1:  return RenderConfig::MSAALevel::None;
    case 2:  return RenderConfig::MSAALevel::x2;
    case 4:  return RenderConfig::MSAALevel::x4;
    case 8:  return RenderConfig::MSAALevel::x8;
    default: return RenderConfig::MSAALevel::None;
    }
}

void EngineSettings::save() {
    m_qsettings.setValue("backend",     backendToInt(m_backend));
    m_qsettings.setValue("msaa",        msaaToInt(m_msaa));
    m_qsettings.setValue("vsync",       m_vsync);
}

void EngineSettings::load() {
    m_backend = intToBackend(m_qsettings.value("backend", backendToInt(GraphicsBackend::Vulkan)).toInt());
    m_msaa    = intToMsaa(m_qsettings.value("msaa", 1).toInt());
    m_vsync   = m_qsettings.value("vsync", true).toBool();
}
