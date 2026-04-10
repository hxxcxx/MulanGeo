#pragma once
#include <string>

namespace MulanGeo {
namespace Core {

class Texture {
public:
    Texture() = default;
    ~Texture() { destroy(); }

    void create(const std::string& path);
    void create(int width, int height, unsigned char* data);
    void destroy();
    void bind(int slot = 0) const;
    void unbind() const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    unsigned int m_id = 0;
    int m_width = 0, m_height = 0;
};

}
}
