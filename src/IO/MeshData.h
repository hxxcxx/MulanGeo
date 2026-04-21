/**
 * @file MeshData.h
 * @brief 导入结果定义
 * @author hxxcxx
 * @date 2026-04-21
 */
#pragma once

#include "MulanGeo/Engine/Geometry/MeshGeometry.h"

#include <memory>
#include <string>
#include <vector>

namespace MulanGeo::IO {

// 一个零件（对应场景中的一个 GeometryNode）
struct Part {
    std::string name;
    std::vector<std::unique_ptr<Engine::MeshGeometry>> faceMeshes; // 按面拆分的网格
};

struct ImportResult {
    std::vector<Part> parts;
    std::string sourceFile;
    std::string error;
    bool success = false;
};

} // namespace MulanGeo::IO
