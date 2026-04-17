/**
 * @file MainWindow.cpp
 * @brief Qt主窗口实现 — 欢迎页 + 按需创建渲染视口
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
#include <QStackedWidget>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

// ==================== MainWindow ====================

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("MulanGeo");
    resize(1280, 720);
    setAcceptDrops(true);

    // Stacked widget: page 0 = welcome, page 1 = render view
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);
    showWelcomePage();

    // Menu
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* openAction = fileMenu->addAction("&Open...");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    // Toolbar
    auto* toolbar = addToolBar("Main");
    toolbar->addAction(openAction);

    statusBar()->showMessage("Ready");
}

void MainWindow::showWelcomePage() {
    // Remove old render widget if exists
    if (m_renderWidget) {
        m_stack->removeWidget(m_renderWidget);
        delete m_renderWidget;
        m_renderWidget = nullptr;
    }

    // Welcome page (simple label, can be replaced with fancy QWidget later)
    auto* welcome = new QLabel(
        "<h2 style='color:#888;'>MulanGeo</h2>"
        "<p style='color:#aaa;'>Open a CAD file to begin — File → Open, or drag & drop</p>",
        this);
    welcome->setAlignment(Qt::AlignCenter);
    welcome->setStyleSheet("background-color: #2b2b2b;");
    m_stack->addWidget(welcome);
    m_stack->setCurrentWidget(welcome);
}

void MainWindow::showRenderView(const MulanGeo::IO::ImportResult& result) {
    // Remove welcome page(s) — everything that isn't the render widget
    while (m_stack->count() > 0 && m_stack->widget(0) != m_renderWidget) {
        auto* w = m_stack->widget(0);
        m_stack->removeWidget(w);
        delete w;
    }

    // Create render widget on demand
    m_renderWidget = new RenderWidget(this);
    m_stack->addWidget(m_renderWidget);
    m_stack->setCurrentWidget(m_renderWidget);

    m_renderWidget->loadMesh(result);
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

    showRenderView(result);

    statusBar()->showMessage(
        QString("Loaded: %1 | Meshes: %2 | Vertices: %3 | Triangles: %4")
            .arg(QFileInfo(filePath).fileName())
            .arg(result.meshes.size())
            .arg(totalVerts)
            .arg(totalTris)
    );
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    const auto urls = e->mimeData()->urls();
    if (urls.isEmpty()) return;
    // TODO: trigger file open with urls[0].toLocalFile()
}
