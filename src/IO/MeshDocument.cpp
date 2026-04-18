/**
 * @file MeshDocument.cpp
 * @brief MeshDocument 实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#include "MulanGeo/IO/MeshDocument.h"

#include <filesystem>

namespace MulanGeo::IO {

std::unique_ptr<MeshDocument> MeshDocument::fromImportResult(
    ImportResult result, std::string filePath)
{
    auto doc = std::make_unique<MeshDocument>();
    doc->m_filePath    = std::move(filePath);
    doc->m_displayName = std::filesystem::path(doc->m_filePath).filename().string();
    doc->m_data        = std::move(result);
    return doc;
}

std::string MeshDocument::summary() const {
    size_t verts = 0, tris = 0;
    for (const auto& m : m_data.meshes) {
        verts += m.vertices.size();
        tris  += m.indices.size() / 3;
    }
    return std::to_string(m_data.meshes.size()) + " parts | "
         + std::to_string(verts) + " verts | "
         + std::to_string(tris) + " tris";
}

} // namespace MulanGeo::IO
