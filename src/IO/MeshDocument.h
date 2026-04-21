/**
 * @file MeshDocument.h
 * @brief 网格文档 — 持有导入的网格数据
 * @author hxxcxx
 * @date 2026-04-21
 */
#pragma once

#include "Document.h"
#include "MeshData.h"

#include <memory>

namespace MulanGeo::IO {

class IO_API MeshDocument final : public Document {
public:
    MeshDocument() = default;
    MeshDocument(const MeshDocument&) = delete;
    MeshDocument& operator=(const MeshDocument&) = delete;

    std::string typeName() const override { return "mesh"; }
    std::string summary() const override;

    /// 从 ImportResult 构造
    static std::unique_ptr<MeshDocument> fromImportResult(
        ImportResult result, std::string filePath);

    /// 访问零件数据
    const std::vector<Part>& parts() const { return m_parts; }
    std::vector<Part>& parts() { return m_parts; }

private:
    std::vector<Part> m_parts;
};

} // namespace MulanGeo::IO
