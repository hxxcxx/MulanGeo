#pragma once
#include <memory>
#include "MulanGeo/Core/Window.h"
#include "MulanGeo/Core/Renderer.h"
#include "MulanGeo/Core/Shader.h"
#include "MulanGeo/Core/Mesh.h"

namespace MulanGeo {
namespace App {

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void setupShaders();
    void setupCube();

    std::unique_ptr<Core::Window> m_window;
    Core::Renderer m_renderer;
    Core::Shader m_shader;
    Core::Mesh m_cubeMesh;
};

}
}
