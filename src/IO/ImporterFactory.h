/**
 * @file ImporterFactory.h
 * @brief 文件导入器工厂，按扩展名创建对应导入器
 * @author hxxcxx
 * @date 2026-04-15
 */
#pragma once
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include "IFileImporter.h"

namespace MulanGeo::IO {

class IO_API ImporterFactory {
public:
    using Creator = std::function<std::unique_ptr<IFileImporter>()>;

    static ImporterFactory& instance();

    void registerImporter(const std::string& extension, Creator creator);

    std::unique_ptr<IFileImporter> create(const std::string& extension) const;

    std::vector<std::string> allSupportedExtensions() const;

    ImporterFactory(const ImporterFactory&) = delete;
    ImporterFactory& operator=(const ImporterFactory&) = delete;

private:
    ImporterFactory() = default;
    std::unordered_map<std::string, Creator> m_creators;
};

class IO_API AutoRegisterImporter {
public:
    AutoRegisterImporter(const std::string& extension, ImporterFactory::Creator creator);
};

} // namespace MulanGeo::IO
