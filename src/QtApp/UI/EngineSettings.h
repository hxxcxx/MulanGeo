/**
 * @file EngineSettings.h
 * @brief 引擎渲染设置单例 — 持久化 ViewConfig 中 UI 可控的部分
 * @author hxxcxx
 * @date 2026-04-24
 */
#pragma once

#include <MulanGeo/Engine/Render/EngineView.h>

#include <QSettings>

class EngineSettings {
public:
    static EngineSettings& instance();

    /// 后端
    MulanGeo::Engine::GraphicsBackend backend() const;
    void setBackend(MulanGeo::Engine::GraphicsBackend b);

    /// 抗锯齿
    MulanGeo::Engine::RenderConfig::MSAALevel msaa() const;
    void setMsaa(MulanGeo::Engine::RenderConfig::MSAALevel level);

    /// VSync
    bool vsync() const;
    void setVsync(bool v);

    /// 填充 ViewConfig 中 UI 可控的字段（不影响原生窗口句柄等）
    void applyTo(MulanGeo::Engine::ViewConfig& cfg) const;

    /// 从 ViewConfig 读取（用于首次保存默认值以外的场景）
    void loadFrom(const MulanGeo::Engine::ViewConfig& cfg);

private:
    EngineSettings();
    ~EngineSettings() = default;
    EngineSettings(const EngineSettings&) = delete;
    EngineSettings& operator=(const EngineSettings&) = delete;

    void save();
    void load();

    QSettings m_qsettings{"MulanGeo", "Engine"};

    MulanGeo::Engine::GraphicsBackend           m_backend = MulanGeo::Engine::GraphicsBackend::Vulkan;
    MulanGeo::Engine::RenderConfig::MSAALevel   m_msaa    = MulanGeo::Engine::RenderConfig::MSAALevel::None;
    bool                                        m_vsync   = true;
};
