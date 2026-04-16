/**
 * @file IFileImporter.h
 * @brief 文件导入器接口定义
 * @author hxxcxx
 * @date 2026-04-15
 */
#pragma once
#include <string>
#include <vector>
#include <memory>

#ifdef BUILDING_IO
  #define IO_API __declspec(dllexport)
#else
  #define IO_API __declspec(dllimport)
#endif

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
