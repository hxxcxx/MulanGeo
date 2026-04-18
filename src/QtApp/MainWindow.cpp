/**
 * @file MainWindow.cpp
 * @brief Qt主窗口实现 — 欢迎页 + 按需创建渲染视口
 * @author hxxcxx
 * @date 2026-04-16
 */

#include "MainWindow.h"
#include "RenderWidget.h"

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
#include <QFileInfo>

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

void MainWindow::showRenderView(MulanGeo::IO::UIDocument* uiDoc) {
    // Remove welcome page(s) — everything that isn't the render widget
    while (m_stack->count() > 0 && m_stack->widget(0) != m_renderWidget) {
        auto* w = m_stack->widget(0);
        m_stack->removeWidget(w);
        delete w;
    }

    // Create render widget on demand
    if (!m_renderWidget) {
        m_renderWidget = new RenderWidget(this);
        m_stack->addWidget(m_renderWidget);
    }
    m_stack->setCurrentWidget(m_renderWidget);

    m_renderWidget->setUIDocument(uiDoc);
}

void MainWindow::onOpenFile() {
    auto exts = m_docManager.supportedExtensions();
    QString filter = "CAD Files (";
    for (const auto& ext : exts) {
        filter += QString(" *.%1").arg(QString::fromStdString(ext));
    }
    filter += " )";

    QString filePath = QFileDialog::getOpenFileName(this, "Open CAD File", {}, filter);
    if (filePath.isEmpty()) return;

    statusBar()->showMessage("Loading: " + filePath);

    auto* uiDoc = m_docManager.openFile(filePath.toStdString());
    if (!uiDoc) {
        QMessageBox::warning(this, "Import Error",
            QString::fromStdString(m_docManager.lastError()));
        statusBar()->showMessage("Ready");
        return;
    }

    showRenderView(uiDoc);

    statusBar()->showMessage(
        QString("Loaded: %1 | %2")
            .arg(QString::fromStdString(uiDoc->document().displayName()))
            .arg(QString::fromStdString(uiDoc->document().summary())));
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    const auto urls = e->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString filePath = urls[0].toLocalFile();
    if (filePath.isEmpty()) return;

    statusBar()->showMessage("Loading: " + filePath);

    auto* uiDoc = m_docManager.openFile(filePath.toStdString());
    if (!uiDoc) {
        QMessageBox::warning(this, "Import Error",
            QString::fromStdString(m_docManager.lastError()));
        statusBar()->showMessage("Ready");
        return;
    }

    showRenderView(uiDoc);

    statusBar()->showMessage(
        QString("Loaded: %1 | %2")
            .arg(QString::fromStdString(uiDoc->document().displayName()))
            .arg(QString::fromStdString(uiDoc->document().summary())));
}
