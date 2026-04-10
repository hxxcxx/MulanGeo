#include "MulanGeo/App/Application.h"
#include <glad/glad.h>

namespace MulanGeo {
namespace App {

Application::Application() : m_window(std::make_unique<Core::Window>(1280, 720, "MulanGeo")) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    setupShaders();
    setupCube();
}

Application::~Application() = default;

void Application::run() {
    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        m_window->makeContextCurrent();

        m_renderer.clear();
        m_renderer.setViewport(m_window->getWidth(), m_window->getHeight());

        m_shader.use();
        m_cubeMesh.draw();

        m_window->swapBuffers();
    }
}

void Application::setupShaders() {
    const char* vertexShader = R"(
        #version 460
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 uMVP;

        out vec3 vNormal;
        out vec3 vPos;
        out vec2 vTexCoord;

        void main() {
            gl_Position = uMVP * vec4(aPos, 1.0);
            vNormal = aNormal;
            vPos = aPos;
            vTexCoord = aTexCoord;
        }
    )";

    const char* fragmentShader = R"(
        #version 460
        in vec3 vNormal;
        in vec3 vPos;
        in vec2 vTexCoord;

        out vec4 FragColor;

        uniform vec3 uColor;

        void main() {
            vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
            float diff = max(dot(normalize(vNormal), lightDir), 0.0);
            vec3 ambient = vec3(0.2);
            FragColor = vec4(uColor * (ambient + diff), 1.0);
        }
    )";

    m_shader.compile(vertexShader, fragmentShader);
    m_shader.setVec3("uColor", 0.8f, 0.6f, 0.2f);
}

void Application::setupCube() {
    std::vector<Core::Vertex> vertices = {
        // front
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}},
        // back
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1}},
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,  2, 3, 0,
        4, 5, 6,  6, 7, 4,
        1, 5, 6,  6, 2, 1,
        0, 4, 7,  7, 3, 0,
        3, 2, 6,  6, 7, 3,
        0, 1, 5,  5, 4, 0,
    };

    m_cubeMesh.setData(vertices, indices);
}

}
}
