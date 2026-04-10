#pragma once
#include <string>
#include <unordered_map>

namespace MulanGeo {
namespace Core {

class Shader {
public:
    Shader() = default;
    ~Shader();

    void compile(const std::string& vertexSrc, const std::string& fragmentSrc);
    void use() const;
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, float x, float y, float z);
    void setMat4(const std::string& name, const float* value);

    unsigned int getID() const { return m_rendererID; }

private:
    unsigned int m_rendererID = 0;
    std::unordered_map<std::string, int> m_uniformLocationCache;

    int getUniformLocation(const std::string& name);
    unsigned int compileShader(unsigned int type, const std::string& source);
};

}
}
