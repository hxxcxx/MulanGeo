#include <GLFW/glfw3.h>
#include "MulanGeo/App/Application.h"

int main() {
    if (!glfwInit()) {
        return -1;
    }

    {
        MulanGeo::App::Application app;
        app.run();
    }

    glfwTerminate();
    return 0;
}
