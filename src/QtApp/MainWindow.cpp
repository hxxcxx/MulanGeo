#include "MainWindow.h"

#include <MulanGeo/IO/IFileImporter.h>
#include <MulanGeo/IO/ImporterFactory.h>
#include <MulanGeo/IO/MeshData.h>

#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QStatusBar>
#include <QMessageBox>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>

#include <cmath>
#include <algorithm>

// ==================== GLWidget ====================

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget() {
    makeCurrent();
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    doneCurrent();
}

void GLWidget::loadMesh(const MulanGeo::IO::ImportResult& result) {
    m_vertices.clear();
    m_indices.clear();
    m_vertexCount = 0;

    uint32_t indexOffset = 0;
    for (const auto& part : result.meshes) {
        for (const auto& v : part.vertices) {
            m_vertices.push_back(v.position.x);
            m_vertices.push_back(v.position.y);
            m_vertices.push_back(v.position.z);
            m_vertices.push_back(v.normal.x);
            m_vertices.push_back(v.normal.y);
            m_vertices.push_back(v.normal.z);
        }
        for (auto idx : part.indices) {
            m_indices.push_back(indexOffset + idx);
        }
        indexOffset += static_cast<uint32_t>(part.vertices.size());
    }

    m_vertexCount = static_cast<int>(m_indices.size());
    m_meshReady = true;

    // Auto-fit camera
    float maxCoord = 0;
    for (size_t i = 0; i < m_vertices.size(); i += 6) {
        maxCoord = std::max({maxCoord, std::abs(m_vertices[i]), std::abs(m_vertices[i+1]), std::abs(m_vertices[i+2])});
    }
    m_zoom = maxCoord * 2.5f;
    m_panX = m_panY = 0;
    m_rotX = 30;
    m_rotY = 45;

    m_needUpload = true;
    update();
}

void GLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    setupShaders();
}

void GLWidget::setupShaders() {
    const char* vertSrc = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 uMVP;
        uniform mat4 uModel;

        out vec3 vNormal;
        out vec3 vWorldPos;

        void main() {
            gl_Position = uMVP * vec4(aPos, 1.0);
            vNormal = normalize(mat3(uModel) * aNormal);
            vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
        }
    )";

    const char* fragSrc = R"(
        #version 450 core
        in vec3 vNormal;
        in vec3 vWorldPos;
        out vec4 FragColor;

        uniform vec3 uLightDir;
        uniform vec3 uCameraPos;

        void main() {
            vec3 N = normalize(vNormal);
            vec3 L = normalize(uLightDir);
            vec3 V = normalize(uCameraPos - vWorldPos);
            vec3 H = normalize(L + V);

            // Base color - neutral grey for CAD
            vec3 baseColor = vec3(0.85, 0.85, 0.88);

            // Two-light setup
            // Key light
            float diff1 = max(dot(N, L), 0.0);
            // Fill light (opposite side, softer)
            float diff2 = max(dot(N, normalize(vec3(-0.3, 0.5, -0.4))), 0.0) * 0.3;

            float ambient = 0.15;
            float diffuse = diff1 * 0.65 + diff2;
            float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.4;

            // Subtle rim light to show silhouette
            float rim = 1.0 - max(dot(V, N), 0.0);
            rim = smoothstep(0.55, 1.0, rim) * 0.15;

            vec3 color = baseColor * (ambient + diffuse) + vec3(spec) + vec3(rim);
            FragColor = vec4(color, 1.0);
        }
    )";

    auto compile = [this](GLenum type, const char* src) -> GLuint {
        GLuint shader = this->glCreateShader(type);
        this->glShaderSource(shader, 1, &src, nullptr);
        this->glCompileShader(shader);
        GLint ok;
        this->glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            this->glGetShaderInfoLog(shader, 512, nullptr, log);
            qWarning("Shader error: %s", log);
        }
        return shader;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vs);
    glAttachShader(m_shaderProgram, fs);
    glLinkProgram(m_shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void GLWidget::uploadMesh() {
    if (m_vertices.empty()) return;

    // Clean up old GL objects
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t), m_indices.data(), GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_needUpload) {
        uploadMesh();
        m_needUpload = false;
    }

    if (!m_meshReady || !m_shaderProgram || !m_vao || m_vertexCount <= 0) return;

    glUseProgram(m_shaderProgram);

    float aspect = static_cast<float>(width()) / std::max(height(), 1);
    float nearP = 0.01f, farP = 1000.0f;
    float fov = 45.0f;
    float t = tanf(fov * 3.14159265f / 360.0f);

    // Perspective matrix
    float proj[16] = {};
    proj[0] = 1.0f / (aspect * t);
    proj[5] = 1.0f / t;
    proj[10] = -(farP + nearP) / (farP - nearP);
    proj[11] = -1.0f;
    proj[14] = -2.0f * farP * nearP / (farP - nearP);

    float rx = m_rotX * 3.14159265f / 180.0f;
    float ry = m_rotY * 3.14159265f / 180.0f;
    float crx = cosf(rx), srx = sinf(rx);
    float cry = cosf(ry), sry = sinf(ry);

    // View: rotate then translate back
    float view[16] = {};
    view[0]  = cry;       view[2]  = sry;
    view[4]  = srx*sry;   view[5]  = crx;   view[6]  = -srx*cry;
    view[8]  = -crx*sry;  view[9]  = srx;   view[10] = crx*cry;
    view[14] = -m_zoom;
    view[15] = 1.0f;

    // MVP = proj * view
    float mvp[16] = {};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                mvp[j*4+i] += proj[k*4+i] * view[j*4+k];

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uMVP"), 1, GL_FALSE, mvp);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uModel"), 1, GL_FALSE, view);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "uLightDir"), 0.5f, 0.8f, 0.6f);
    // Camera position in world space: inverse of view rotation * (0,0,zoom)
    float camX = sry * crx * m_zoom;
    float camY = srx * m_zoom;
    float camZ = cry * crx * m_zoom;
    glUniform3f(glGetUniformLocation(m_shaderProgram, "uCameraPos"), camX, camY, camZ);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_vertexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void GLWidget::mousePressEvent(QMouseEvent* e) {
    m_lastMousePos = e->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent* e) {
    int dx = e->pos().x() - m_lastMousePos.x();
    int dy = e->pos().y() - m_lastMousePos.y();

    if (e->buttons() & Qt::LeftButton) {
        m_rotY += dx * 0.5f;
        m_rotX += dy * 0.5f;
    } else if (e->buttons() & Qt::RightButton) {
        m_panX += dx * m_zoom * 0.002f;
        m_panY -= dy * m_zoom * 0.002f;
    }
    m_lastMousePos = e->pos();
    update();
}

void GLWidget::wheelEvent(QWheelEvent* e) {
    m_zoom -= e->angleDelta().y() * m_zoom * 0.001f;
    m_zoom = std::max(0.01f, m_zoom);
    update();
}

// ==================== MainWindow ====================

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("MulanGeo");
    resize(1280, 720);

    m_glWidget = new GLWidget(this);
    setCentralWidget(m_glWidget);

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

    // Build filter string from registered importers
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

    m_glWidget->loadMesh(result);

    statusBar()->showMessage(
        QString("Loaded: %1 | Meshes: %2 | Vertices: %3 | Triangles: %4")
            .arg(QFileInfo(filePath).fileName())
            .arg(result.meshes.size())
            .arg(totalVerts)
            .arg(totalTris)
    );
}
