#pragma once
#include <vector>
#include <memory>

namespace MulanGeo {
namespace Core {

struct Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
};

class Mesh {
public:
    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    void setData(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    void bind() const;
    void unbind() const;
    void draw() const;

    unsigned int getIndexCount() const { return m_indexCount; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    unsigned int m_indexCount = 0;
};

}
}
