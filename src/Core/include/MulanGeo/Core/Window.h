#pragma once
#include <string>
#include <memory>

struct GLFWwindow;

namespace MulanGeo {
namespace Core {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    void makeContextCurrent();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    GLFWwindow* getNativeHandle() { return m_window; }

private:
    GLFWwindow* m_window;
    int m_width, m_height;
};

}
}
