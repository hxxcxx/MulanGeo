#include "MulanGeo/Core/Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace MulanGeo {
namespace Core {

Shader::~Shader() {
    if (m_rendererID) glDeleteProgram(m_rendererID);
}

void Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc) {
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    m_rendererID = glCreateProgram();
    glAttachShader(m_rendererID, vertex);
    glAttachShader(m_rendererID, fragment);
    glLinkProgram(m_rendererID);

    int success;
    glGetProgramiv(m_rendererID, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(m_rendererID, 512, nullptr, info);
        throw std::runtime_error("Shader program linking failed: " + std::string(info));
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

int Shader::getUniformLocation(const std::string& name) {
    if (m_uniformLocationCache.count(name)) return m_uniformLocationCache[name];
    int loc = glGetUniformLocation(m_rendererID, name.c_str());
    m_uniformLocationCache[name] = loc;
    return loc;
}

unsigned int Shader::compileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(id, 512, nullptr, info);
        throw std::runtime_error("Shader compilation failed: " + std::string(info));
    }
    return id;
}

void Shader::use() const {
    glUseProgram(m_rendererID);
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setMat4(const std::string& name, const float* value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, value);
}

}
}
