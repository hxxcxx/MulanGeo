/**
 * @file RenderWidget.h
 * @brief Qt 渲染控件 — EngineView 的薄壳封装
 * @author hxxcxx
 * @date 2026-04-17
 *
 * 职责（仅限 Qt 平台胶水）：
 * - 将 Qt 事件翻译为 Engine::InputEvent
 * - 驱动 EngineView 的生命周期（init / resize / renderFrame）
 * - 不直接接触任何 RHI 类型
 */

#pragma once

#include <QWidget>
#include <QTimer>

#include <MulanGeo/Engine/Render/EngineView.h>
#include <MulanGeo/Engine/Interaction/InputEvent.h>

#include <memory>

namespace MulanGeo::IO {
struct ImportResult;
}

// ============================================================
// RenderWidget
// ============================================================

class RenderWidget : public QWidget {
    Q_OBJECT

public:
    explicit RenderWidget(QWidget* parent = nullptr);
    ~RenderWidget();

    /// 加载 mesh 数据（委托给 EngineView）
    void loadMesh(const MulanGeo::IO::ImportResult& result);

    /// 请求渲染下一帧
    void requestFrame();

    /// 访问底层引擎视图（高级用法：切换 Operator 等）
    MulanGeo::Engine::EngineView& engineView() { return m_view; }

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void paintEvent(QPaintEvent*) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private:
    /// 将 Qt 鼠标按钮映射为 Engine::MouseButton
    static MulanGeo::Engine::MouseButton translateButton(Qt::MouseButton btn);

    /// 将 Qt 按钮组合映射为 Engine::MouseButton 位掩码
    static MulanGeo::Engine::MouseButton translateButtons(Qt::MouseButtons btns);

    /// 将 Qt 键盘修饰键映射
    static MulanGeo::Engine::KeyModifier translateModifiers(Qt::KeyboardModifiers mods);

    /// 将 Qt 按键映射
    static MulanGeo::Engine::Key translateKey(int qtKey);

    MulanGeo::Engine::EngineView  m_view;
    QTimer*                       m_timer = nullptr;
};
