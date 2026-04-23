/**
 * @file MainWindow.cpp
 * @brief Qt 主窗口实现
 * @author hxxcxx
 * @date 2026-04-22
 */
#include "MainWindow.h"
#include "DocWidget.h"
#include "UIDocument.h"

#include <MulanGeo/Document/Document.h>

#include <QFileDialog>
#include <QStatusBar>
#include <QStackedWidget>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget* parent) : SARibbonMainWindow(parent) {
    setWindowTitle("MulanGeo");
    resize(1280, 720);
    setAcceptDrops(true);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);
    showWelcomePage();

    auto* ribbonBar = this->ribbonBar();
    ribbonBar->setRibbonStyle(SARibbonBar::RibbonStyleCompact);

    auto* categoryFile = ribbonBar->addCategoryPage(tr("Home"));

    auto* panelFile = new SARibbonPanel(tr("File"), categoryFile);
    auto* openAction = new QAction(tr("Open"), this);
    panelFile->addLargeAction(openAction);
    categoryFile->addPanel(panelFile);

    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    statusBar()->showMessage("Ready");
}

void MainWindow::showWelcomePage() {
    if (m_renderWidget) {
        m_stack->removeWidget(m_renderWidget);
        delete m_renderWidget;
        m_renderWidget = nullptr;
    }
    m_uiDoc.reset();

    auto* welcome = new QLabel(
        "<h2 style='color:#888;'>MulanGeo</h2>"
        "<p style='color:#aaa;'>Open a CAD file to begin: File → Open, or drag & drop</p>",
        this);
    welcome->setAlignment(Qt::AlignCenter);
    welcome->setStyleSheet("background-color: #2b2b2b;");
    m_stack->addWidget(welcome);
    m_stack->setCurrentWidget(welcome);
}

void MainWindow::showRenderView(UIDocument* uiDoc) {
    while (m_stack->count() > 0 && m_stack->widget(0) != m_renderWidget) {
        auto* w = m_stack->widget(0);
        m_stack->removeWidget(w);
        delete w;
    }

    if (!m_renderWidget) {
        m_renderWidget = new DocWidget(this);
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

    auto* doc = m_docManager.openFile(filePath.toStdString());
    if (!doc) {
        QMessageBox::warning(this, "Import Error",
            QString::fromStdString(m_docManager.lastError()));
        statusBar()->showMessage("Ready");
        return;
    }

    m_uiDoc = std::make_unique<UIDocument>(doc);
    showRenderView(m_uiDoc.get());

    statusBar()->showMessage(
        QString("Loaded: %1 | %2")
            .arg(QString::fromStdString(doc->displayName()))
            .arg(QString::fromStdString(doc->summary())));
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

    auto* doc = m_docManager.openFile(filePath.toStdString());
    if (!doc) {
        QMessageBox::warning(this, "Import Error",
            QString::fromStdString(m_docManager.lastError()));
        statusBar()->showMessage("Ready");
        return;
    }

    m_uiDoc = std::make_unique<UIDocument>(doc);
    showRenderView(m_uiDoc.get());

    statusBar()->showMessage(
        QString("Loaded: %1 | %2")
            .arg(QString::fromStdString(doc->displayName()))
            .arg(QString::fromStdString(doc->summary())));
}
