/**
 * @file MainWindow.h
 * @brief Qt主窗口 — 欢迎页 + 按需创建渲染视口
 * @author hxxcxx
 * @date 2026-04-16
 */
#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <MulanGeo/IO/DocumentManager.h>

class RenderWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();

private:
    void showWelcomePage();
    void showRenderView(MulanGeo::IO::UIDocument* uiDoc);

    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    QStackedWidget* m_stack = nullptr;
    RenderWidget*   m_renderWidget = nullptr;
    MulanGeo::IO::DocumentManager m_docManager;
};
