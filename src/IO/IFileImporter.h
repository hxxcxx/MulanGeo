/**
 * @file IFileImporter.h
 * @brief 文件导入器接口定义
 * @author hxxcxx
 * @date 2026-04-15
 */
#pragma once

#include "IOExport.h"

#include <string>
#include <vector>
#include <memory>

namespace MulanGeo::IO {

struct ImportResult;

class IO_API IFileImporter {
public:
    virtual ~IFileImporter() = default;

    virtual ImportResult importFile(const std::string& path) = 0;

    virtual std::vector<std::string> supportedExtensions() const = 0;
    virtual std::string name() const = 0;
};

} // namespace MulanGeo::IO
