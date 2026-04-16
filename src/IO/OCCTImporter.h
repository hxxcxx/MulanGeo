/**
 * @file OCCTImporter.h
 * @brief OpenCASCADE文件导入器实现
 * @author hxxcxx
 * @date 2026-04-15
 */
#pragma once
#include "IFileImporter.h"

namespace MulanGeo::IO {

class OCCTImporter : public IFileImporter {
public:
    ImportResult importFile(const std::string& path) override;
    std::vector<std::string> supportedExtensions() const override;
    std::string name() const override;
};

} // namespace MulanGeo::IO
