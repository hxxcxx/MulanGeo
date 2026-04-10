#include "MulanGeo/Core/Buffer.h"
#include <glad/glad.h>

namespace MulanGeo {
namespace Core {

void Buffer::create(float* data, size_t size) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void Buffer::destroy() {
    if (m_id) glDeleteBuffers(1, &m_id);
}

void Buffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void Buffer::unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}
}
