/**
 * @file MainWindow.h
 * @brief Qt主窗口 — 欢迎页 + 按需创建渲染视口
 * @author hxxcxx
 * @date 2026-04-16
 */
#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <memory>

class RenderWidget;

namespace MulanGeo::IO {
struct ImportResult;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();

private:
    void showWelcomePage();
    void showRenderView(const MulanGeo::IO::ImportResult& result);

    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    QStackedWidget* m_stack = nullptr;
    RenderWidget*   m_renderWidget = nullptr;
};
