#include "MulanGeo/Core/Window.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdexcept>

namespace MulanGeo {
namespace Core {

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwSwapInterval(1);
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::makeContextCurrent() {
    glfwMakeContextCurrent(m_window);
}

}
}
