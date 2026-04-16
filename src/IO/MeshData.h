/**
 * @file MeshData.h
 * @brief 网格数据结构定义，顶点、索引与子网格
 * @author hxxcxx
 * @date 2026-04-15
 */
#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace MulanGeo::IO {

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

struct Vec2 {
    float u = 0, v = 0;
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
};

struct MeshPart {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
};

struct ImportResult {
    std::vector<MeshPart> meshes;
    std::string sourceFile;
    std::string error;
    bool success = false;
};

} // namespace MulanGeo::IO
