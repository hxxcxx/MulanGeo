cmake_minimum_required(VERSION 3.20)
project(MulanGeo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 依赖管理 - vcpkg
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# glad (OpenGL loader)
find_package(glad QUIET)
if(glad_FOUND)
    message(STATUS "glad found")
else()
    message(FATAL_ERROR "glad not found, please install via vcpkg")
endif()

# 子目录
add_subdirectory(src/Core)
add_subdirectory(src/App)

# 配置头文件
configure_file(
    ${CMAKE_SOURCE_DIR}/include/MulanGeo/Version.h.in
    ${CMAKE_BINARY_DIR}/include/MulanGeo/Version.h
)
