#include "IO/CADImporter.h"
#include <filesystem>
#include <fstream>

namespace MulanGeo {
namespace IO {

CADImporter::CADImporter() = default;
CADImporter::~CADImporter() = default;

std::vector<Core::Mesh> CADImporter::import(const std::string& filePath) {
    CADFormat fmt = detectFormat(filePath);

    switch (fmt) {
        case CADFormat::STEP: return importStep(filePath);
        case CADFormat::OBJ:  return importObj(filePath);
        case CADFormat::DWF:  return importDwf(filePath);
        default: return {};
    }
}

CADFormat CADImporter::detectFormat(const std::string& filePath) {
    std::filesystem::path p(filePath);
    std::string ext = p.extension().string();

    if (ext == ".step" || ext == ".stp") return CADFormat::STEP;
    if (ext == ".iges" || ext == ".igs") return CADFormat::IGES;
    if (ext == ".obj") return CADFormat::OBJ;
    if (ext == ".dwf") return CADFormat::DWF;

    return CADFormat::Unknown;
}

std::vector<Core::Mesh> CADImporter::importStep(const std::string& filePath) {
#ifdef HAVE_OCC
    // TODO: OCCT STEP导入实现
    // 1. STEPControl_Reader 读取文件
    // 2. BRep_Builder 构建拓扑
    // 3. TopoDS_Shape 转换为网格
#else
    return {};
#endif
}

std::vector<Core::Mesh> CADImporter::importObj(const std::string& filePath) {
    std::vector<Core::Mesh> meshes;
    std::vector<Core::Vertex> vertices;
    std::vector<unsigned int> indices;

    std::ifstream file(filePath);
    if (!file.is_open()) return meshes;

    std::string line;
    while (std::getline(file, line)) {
        // 简单的OBJ解析 - 后续完善
    }

    if (!vertices.empty()) {
        meshes.push_back(Core::Mesh(vertices, indices));
    }

    return meshes;
}

std::vector<Core::Mesh> CADImporter::importDwf(const std::string& filePath) {
    // TODO: DWF格式支持
    return {};
}

}
}
