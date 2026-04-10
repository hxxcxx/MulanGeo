# Core模块 - GL渲染引擎核心
add_library(Core STATIC
    src/Window.cpp
    src/Shader.cpp
    src/Buffer.cpp
    src/Texture.cpp
    src/Mesh.cpp
    src/Renderer.cpp
)

target_compile_definitions(Core PUBLIC GLFW_INCLUDE_NONE)

target_link_libraries(Core PUBLIC
    glfw
    glad::glad
    OpenGL::GL
)

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
