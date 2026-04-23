/**
 * @file DocWidget.h
 * @brief Qt 渲染控件 — EngineView 的薄壳封装
 * @author hxxcxx
 * @date 2026-04-22
 */
#pragma once

#include <QWidget>

#include <MulanGeo/Engine/Render/EngineView.h>
#include <MulanGeo/Engine/Interaction/InputEvent.h>

#include <memory>

class UIDocument;

class DocWidget : public QWidget {
    Q_OBJECT

public:
    explicit DocWidget(QWidget* parent = nullptr);
    ~DocWidget();

    /// 设置当前 UI 文档（绑定场景到视图）
    void setUIDocument(UIDocument* doc);

    /// 请求渲染下一帧
    void requestFrame();

    /// 访问底层引擎视图
    MulanGeo::Engine::EngineView& engineView() { return m_view; }

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void paintEvent(QPaintEvent*) override;
    QPaintEngine* paintEngine() const override { return nullptr; }

    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private:
    static MulanGeo::Engine::MouseButton translateButton(Qt::MouseButton btn);
    static MulanGeo::Engine::MouseButton translateButtons(Qt::MouseButtons btns);
    static MulanGeo::Engine::KeyModifier translateModifiers(Qt::KeyboardModifiers mods);
    static MulanGeo::Engine::Key translateKey(int qtKey);

    MulanGeo::Engine::EngineView  m_view;
    UIDocument*                   m_uiDoc = nullptr;
};
