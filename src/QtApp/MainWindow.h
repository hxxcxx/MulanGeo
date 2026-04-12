#pragma once

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <memory>

namespace MulanGeo::IO {
struct ImportResult;
}

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget() override;

    void loadMesh(const MulanGeo::IO::ImportResult& result);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    void setupShaders();
    void uploadMesh();

    // Mesh data
    std::vector<float> m_vertices;  // interleaved: pos(3) + normal(3)
    std::vector<uint32_t> m_indices;
    int m_vertexCount = 0;

    // GL objects
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0, m_shaderProgram = 0;
    bool m_meshReady = false;
    bool m_needUpload = false;

    // Camera
    float m_rotX = 30.0f, m_rotY = 45.0f, m_zoom = 5.0f;
    float m_panX = 0.0f, m_panY = 0.0f;
    QPoint m_lastMousePos;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();

private:
    GLWidget* m_glWidget;
};
