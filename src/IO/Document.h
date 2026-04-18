/**
 * @file Document.h
 * @brief 文档基类 — 数据层的基本单元
 * @author hxxcxx
 * @date 2026-04-18
 */
#pragma once

#include "IOExport.h"

#include <string>

namespace MulanGeo::IO {

class IO_API Document {
public:
    virtual ~Document() = default;

    /// 文档类型名（"mesh", "assembly" 等）
    virtual std::string typeName() const = 0;

    /// 统计摘要（给 StatusBar 显示）
    virtual std::string summary() const = 0;

    const std::string& filePath() const { return m_filePath; }
    const std::string& displayName() const { return m_displayName; }
    bool isModified() const { return m_modified; }

protected:
    std::string m_filePath;
    std::string m_displayName;
    bool m_modified = false;
};

} // namespace MulanGeo::IO
