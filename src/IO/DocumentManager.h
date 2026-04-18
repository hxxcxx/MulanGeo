/**
 * @file DocumentManager.h
 * @brief 文档管理器 — 管理所有打开的文档
 * @author hxxcxx
 * @date 2026-04-18
 */
#pragma once

#include "UIDocument.h"

#include <memory>
#include <vector>
#include <string>

namespace MulanGeo::IO {

class IO_API DocumentManager {
public:
    DocumentManager() = default;
    ~DocumentManager();

    DocumentManager(const DocumentManager&) = delete;
    DocumentManager& operator=(const DocumentManager&) = delete;

    /// 打开文件 → 自动匹配 Importer → 构造 UIDocument
    /// @return 成功返回 UIDocument 指针，失败返回 nullptr
    UIDocument* openFile(const std::string& path);

    /// 获取打开的错误信息（openFile 返回 nullptr 时）
    const std::string& lastError() const { return m_lastError; }

    /// 关闭文档
    void closeDocument(UIDocument* doc);

    /// 当前活跃文档
    UIDocument* activeDocument() const { return m_active; }
    void setActive(UIDocument* doc) { m_active = doc; }

    const std::vector<std::unique_ptr<UIDocument>>& documents() const { return m_documents; }

    /// 所有支持的文件扩展名
    std::vector<std::string> supportedExtensions() const;

private:
    std::vector<std::unique_ptr<UIDocument>> m_documents;
    UIDocument* m_active = nullptr;
    std::string m_lastError;
};

} // namespace MulanGeo::IO
