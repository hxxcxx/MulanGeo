/**
 * @file MainWindow.h
 * @brief Qt 主窗口 — 欢迎页 + 按需创建渲染视口
 * @author hxxcxx
 * @date 2026-04-22
 */
#pragma once

#include <SARibbon.h>
#include <QStackedWidget>
#include <MulanGeo/Document/DocumentManager.h>
#include "UIDocument.h"

#include <memory>

class RenderWidget;

class MainWindow : public SARibbonMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();

private:
    void showWelcomePage();
    void showRenderView(UIDocument* uiDoc);

    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    QStackedWidget* m_stack = nullptr;
    RenderWidget*   m_renderWidget = nullptr;

    MulanGeo::Document::DocumentManager m_docManager;
    std::unique_ptr<UIDocument>         m_uiDoc;
};
