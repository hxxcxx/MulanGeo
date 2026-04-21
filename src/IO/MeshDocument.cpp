/**
 * @file MeshDocument.cpp
 * @brief MeshDocument 实现
 * @author hxxcxx
 * @date 2026-04-21
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
    doc->m_parts       = std::move(result.parts);
    return doc;
}

std::string MeshDocument::summary() const {
    size_t verts = 0, tris = 0, faceCount = 0;
    for (const auto& part : m_parts) {
        for (const auto& face : part.faceMeshes) {
            verts += face->vertexCount();
            tris  += face->triangleCount();
            ++faceCount;
        }
    }
    return std::to_string(m_parts.size()) + " parts | "
         + std::to_string(faceCount) + " faces | "
         + std::to_string(verts) + " verts | "
         + std::to_string(tris) + " tris";
}

} // namespace MulanGeo::IO
