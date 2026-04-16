/**
 * @file MainWindow.h
 * @brief Qt主窗口，集成RHI渲染控件
 * @author hxxcxx
 * @date 2026-04-16
 */
#pragma once

#include <QMainWindow>
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
    RenderWidget* m_renderWidget;
};
