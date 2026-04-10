# MulanGeo

基于OpenGL + OpenCASCADE的CAD查看器引擎

## 模块结构

```
src/
├── Core/      # GL渲染引擎核心 (Window, Shader, Mesh, Renderer)
├── IO/        # CAD文件读取模块 (STEP, OBJ, DWF)
└── App/       # 主程序入口
```

## 依赖

- OpenGL 4.6
- GLFW3
- GLEW
- TBB
- OpenCASCADE (可选)

## 构建

```bash
# 使用vcpkg安装依赖
vcpkg install glfw3 glew tbb opencascade

# CMake构建
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```