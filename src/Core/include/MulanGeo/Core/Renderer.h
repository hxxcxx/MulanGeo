#pragma once
#include <memory>
#include "MulanGeo/Core/Shader.h"
#include "MulanGeo/Core/Mesh.h"

namespace MulanGeo {
namespace Core {

class Renderer {
public:
    Renderer() = default;

    void clear(float r = 0.1f, float g = 0.1f, float b = 0.1f, float a = 1.0f);
    void draw(const Mesh& mesh, const Shader& shader);

    void setViewport(int width, int height);
};

}
}
