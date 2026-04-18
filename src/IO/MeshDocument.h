/**
 * @file MeshDocument.h
 * @brief 网格文档 — 持有导入的网格数据
 * @author hxxcxx
 * @date 2026-04-18
 */
#pragma once

#include "Document.h"
#include "MeshData.h"

#include <memory>

namespace MulanGeo::IO {

class IO_API MeshDocument final : public Document {
public:
    std::string typeName() const override { return "mesh"; }
    std::string summary() const override;

    /// 从 ImportResult 构造
    static std::unique_ptr<MeshDocument> fromImportResult(
        ImportResult result, std::string filePath);

    /// 访问原始导入数据
    const ImportResult& importData() const { return m_data; }

private:
    ImportResult m_data;
};

} // namespace MulanGeo::IO
