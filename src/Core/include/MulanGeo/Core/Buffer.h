#pragma once
#include <vector>

namespace MulanGeo {
namespace Core {

class Buffer {
public:
    Buffer() = default;
    ~Buffer() { destroy(); }

    void create(float* data, size_t size);
    void destroy();

    void bind() const;
    void unbind() const;

private:
    unsigned int m_id = 0;
};

}
}
