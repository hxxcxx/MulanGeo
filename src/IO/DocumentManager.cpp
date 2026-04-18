/**
 * @file DocumentManager.cpp
 * @brief DocumentManager 实现
 * @author hxxcxx
 * @date 2026-04-18
 */
#include "MulanGeo/IO/DocumentManager.h"
#include "MulanGeo/IO/MeshDocument.h"
#include "MulanGeo/IO/ImporterFactory.h"

#include <algorithm>
#include <filesystem>

namespace MulanGeo::IO {

DocumentManager::~DocumentManager() = default;

UIDocument* DocumentManager::openFile(const std::string& path) {
    m_lastError.clear();

    std::string ext = std::filesystem::path(path).extension().string();
    if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    auto importer = ImporterFactory::instance().create(ext);
    if (!importer) {
        m_lastError = "No importer for extension: ." + ext;
        return nullptr;
    }

    auto result = importer->importFile(path);
    if (!result.success) {
        m_lastError = result.error;
        return nullptr;
    }

    auto doc = MeshDocument::fromImportResult(std::move(result), path);
    auto uiDoc = std::make_unique<UIDocument>(std::move(doc));
    uiDoc->rebuildScene();

    auto* ptr = uiDoc.get();
    m_documents.push_back(std::move(uiDoc));
    m_active = ptr;
    return ptr;
}

void DocumentManager::closeDocument(UIDocument* doc) {
    if (!doc) return;
    doc->detachView();
    if (m_active == doc) m_active = nullptr;
    std::erase_if(m_documents, [doc](auto& p) { return p.get() == doc; });
    if (!m_active && !m_documents.empty()) {
        m_active = m_documents.back().get();
    }
}

std::vector<std::string> DocumentManager::supportedExtensions() const {
    return ImporterFactory::instance().allSupportedExtensions();
}

} // namespace MulanGeo::IO
