/**
 * @file JsonArchive.h
 * @brief JSON 格式的 OutputArchive / InputArchive 实现
 *
 * 基于 nlohmann-json 库。
 * 用于调试、小数据、文本可读、网络传输场景。
 */
#pragma once

#include "../CoreExport.h"
#include "Archive.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace MulanGeo::Core {

// ============================================================
// JsonOutputArchive
// ============================================================

class CORE_API JsonOutputArchive final : public OutputArchive {
public:
    /// 写入到内部缓冲区
    JsonOutputArchive();

    /// 写入到文件
    explicit JsonOutputArchive(const std::filesystem::path& filePath);

    ~JsonOutputArchive() override;

    // --- 结构标记 ---

    void beginObject() override;
    void endObject() override;
    void key(std::string_view name) override;
    void beginArray(uint32_t size) override;
    void endArray() override;

    // beginBulkArray / endBulkArray 使用默认退化为 beginArray/endArray

    // --- 原始类型写入 ---

    void write(int32_t value) override;
    void write(int64_t value) override;
    void write(float value) override;
    void write(double value) override;
    void write(bool value) override;
    void write(std::string_view value) override;
    void writeBytes(std::span<const byte> data) override;

    // --- 输出 ---

    /// 获取 JSON 字符串（带缩进，用于调试和文件写入）
    std::string toString(int indent = 2) const;

    /// 直接写入文件
    bool saveToFile(const std::filesystem::path& filePath) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ============================================================
// JsonInputArchive
// ============================================================

class CORE_API JsonInputArchive final : public InputArchive {
public:
    /// 从 JSON 字符串读取
    explicit JsonInputArchive(std::string_view jsonStr);

    /// 从文件读取
    explicit JsonInputArchive(const std::filesystem::path& filePath);

    ~JsonInputArchive() override;

    // --- 结构标记 ---

    ArchiveResult beginObject() override;
    ArchiveResult endObject() override;
    ArchiveResult key(std::string_view name) override;
    ArchiveResult beginArray(uint32_t& outSize) override;
    ArchiveResult endArray() override;

    // --- 原始类型读取 ---

    ArchiveResult read(int32_t& out) override;
    ArchiveResult read(int64_t& out) override;
    ArchiveResult read(float& out) override;
    ArchiveResult read(double& out) override;
    ArchiveResult read(bool& out) override;
    ArchiveResult read(std::string& out) override;
    ArchiveResult readBytes(std::vector<byte>& out) override;

    // --- 字段兼容性（JSON 格式实现真正的 key 查找）---

    bool hasKey(std::string_view name) const override;
    ArchiveResult skipValue() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace MulanGeo::Core
