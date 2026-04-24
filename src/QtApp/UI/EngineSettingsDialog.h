/**
 * @file EngineSettingsDialog.h
 * @brief 引擎设置对话框 — 渲染后端 / 抗锯齿 / VSync
 * @author hxxcxx
 * @date 2026-04-24
 */
#pragma once

#include <QDialog>

class QComboBox;

class EngineSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit EngineSettingsDialog(QWidget* parent = nullptr);

private slots:
    void onAccept();
    void onReject();

private:
    void readSettings();

    QComboBox*  m_comboBackend = nullptr;
    QComboBox*  m_comboMsaa    = nullptr;
};
