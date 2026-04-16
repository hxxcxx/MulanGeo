/**
 * @file MainWindow.cpp
 * @brief Qt主窗口实现 - RHI渲染
 * @author hxxcxx
 * @date 2026-04-16
 */

#include "MainWindow.h"
#include "RenderWidget.h"

#include <MulanGeo/IO/IFileImporter.h>
#include <MulanGeo/IO/ImporterFactory.h>
#include <MulanGeo/IO/MeshData.h>

#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QStatusBar>
#include <QMessageBox>

// ==================== MainWindow ====================

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("MulanGeo");
    resize(1280, 720);

    m_renderWidget = new RenderWidget(this);
    setCentralWidget(m_renderWidget);

    // Menu
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* openAction = fileMenu->addAction("&Open...");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    // Toolbar
    auto* toolbar = addToolBar("Main");
    toolbar->addAction(openAction);

    statusBar()->showMessage("Ready");
}

void MainWindow::onOpenFile() {
    using namespace MulanGeo::IO;

    auto exts = ImporterFactory::instance().allSupportedExtensions();
    QString filter = "CAD Files (";
    for (const auto& ext : exts) {
        filter += QString(" *.%1").arg(QString::fromStdString(ext));
    }
    filter += " )";

    QString filePath = QFileDialog::getOpenFileName(this, "Open CAD File", {}, filter);
    if (filePath.isEmpty()) return;

    statusBar()->showMessage("Loading: " + filePath);

    std::string ext = QFileInfo(filePath).suffix().toStdString();
    auto importer = ImporterFactory::instance().create(ext);
    if (!importer) {
        QMessageBox::warning(this, "Error", QString("No importer for: .%1").arg(QString::fromStdString(ext)));
        statusBar()->showMessage("Ready");
        return;
    }

    ImportResult result = importer->importFile(filePath.toStdString());
    if (!result.success) {
        QMessageBox::warning(this, "Import Error", QString::fromStdString(result.error));
        statusBar()->showMessage("Ready");
        return;
    }

    int totalVerts = 0, totalTris = 0;
    for (const auto& mesh : result.meshes) {
        totalVerts += static_cast<int>(mesh.vertices.size());
        totalTris += static_cast<int>(mesh.indices.size() / 3);
    }

    m_renderWidget->loadMesh(result);

    statusBar()->showMessage(
        QString("Loaded: %1 | Meshes: %2 | Vertices: %3 | Triangles: %4")
            .arg(QFileInfo(filePath).fileName())
            .arg(result.meshes.size())
            .arg(totalVerts)
            .arg(totalTris)
    );
}
