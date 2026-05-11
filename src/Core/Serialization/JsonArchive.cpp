/**
 * @file JsonArchive.cpp
 * @brief JSON 格式 Archive 实现
 */

#include "JsonArchive.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <stdexcept>

namespace MulanGeo::Core {

using json = nlohmann::json;

// ============================================================
// JsonOutputArchive::Impl
// ============================================================

struct JsonOutputArchive::Impl {
    // 节点栈：每个元素是一层 JSON 节点
    // 栈底是根节点
    struct StackEntry {
        json node;                  // 当前层级的 JSON 值
        std::string pendingKey;     // 等待被赋值的 key（由 key() 设置）
        bool isArray = false;       // 当前层级是 array 还是 object
    };

    std::vector<StackEntry> stack;
    std::string filePath;   // 空表示写入内存

    Impl() {
        // 根节点是一个 object
        stack.push_back({json::object(), {}, false});
    }

    json& current() { return stack.back().node; }

    void emitValue(json value) {
        auto& top = stack.back();
        if (top.isArray) {
            top.node.push_back(std::move(value));
        } else if (!top.pendingKey.empty()) {
            top.node[top.pendingKey] = std::move(value);
            top.pendingKey.clear();
        } else {
            // 根节点或异常情况：覆盖当前值
            top.node = std::move(value);
        }
    }
};

// ============================================================
// JsonOutputArchive
// ============================================================

JsonOutputArchive::JsonOutputArchive()
    : m_impl(std::make_unique<Impl>()) {}

JsonOutputArchive::JsonOutputArchive(const std::filesystem::path& filePath)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->filePath = filePath.string();
}

JsonOutputArchive::~JsonOutputArchive() = default;

void JsonOutputArchive::beginObject() {
    m_impl->stack.push_back({json::object(), {}, false});
}

void JsonOutputArchive::endObject() {
    if (m_impl->stack.size() <= 1) return;  // 不能弹出根节点
    auto completed = std::move(m_impl->stack.back());
    m_impl->stack.pop_back();
    m_impl->emitValue(std::move(completed.node));
}

void JsonOutputArchive::key(std::string_view name) {
    m_impl->stack.back().pendingKey = std::string(name);
}

void JsonOutputArchive::beginArray(uint32_t size) {
    m_impl->stack.push_back({json::array(), {}, true});
    (void)size;
}

void JsonOutputArchive::endArray() {
    if (m_impl->stack.size() <= 1) return;
    auto completed = std::move(m_impl->stack.back());
    m_impl->stack.pop_back();
    m_impl->emitValue(std::move(completed.node));
}

void JsonOutputArchive::write(int32_t value) {
    m_impl->emitValue(json(value));
}

void JsonOutputArchive::write(int64_t value) {
    m_impl->emitValue(json(value));
}

void JsonOutputArchive::write(float value) {
    m_impl->emitValue(json(value));
}

void JsonOutputArchive::write(double value) {
    m_impl->emitValue(json(value));
}

void JsonOutputArchive::write(bool value) {
    m_impl->emitValue(json(value));
}

void JsonOutputArchive::write(std::string_view value) {
    m_impl->emitValue(json(std::string(value)));
}

void JsonOutputArchive::writeBytes(std::span<const byte> data) {
    // bytes 存储为 base64 编码的 string
    static const char kBase64Table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        uint32_t n = (static_cast<uint8_t>(data[i]) << 16) |
                     (static_cast<uint8_t>(data[i + 1]) << 8) |
                     static_cast<uint8_t>(data[i + 2]);
        encoded += kBase64Table[(n >> 18) & 0x3F];
        encoded += kBase64Table[(n >> 12) & 0x3F];
        encoded += kBase64Table[(n >> 6) & 0x3F];
        encoded += kBase64Table[n & 0x3F];
    }
    if (i < data.size()) {
        uint32_t n = static_cast<uint8_t>(data[i]) << 16;
        if (i + 1 < data.size()) n |= static_cast<uint8_t>(data[i + 1]) << 8;
        encoded += kBase64Table[(n >> 18) & 0x3F];
        encoded += kBase64Table[(n >> 12) & 0x3F];
        encoded += (i + 1 < data.size()) ? kBase64Table[(n >> 6) & 0x3F] : '=';
        encoded += '=';
    }

    // 写为一个带类型标记的 object，以便 readBytes 识别
    auto& top = m_impl->stack.back();
    if (top.isArray) {
        top.node.push_back(json{{"_bytes", encoded}});
    } else if (!top.pendingKey.empty()) {
        top.node[top.pendingKey] = json{{"_bytes", encoded}};
        top.pendingKey.clear();
    } else {
        top.node = json{{"_bytes", encoded}};
    }
}

std::string JsonOutputArchive::toString(int indent) const {
    if (m_impl->stack.empty()) return "{}";
    return m_impl->stack.front().node.dump(indent);
}

bool JsonOutputArchive::saveToFile(const std::filesystem::path& filePath) const {
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs) return false;
    ofs << toString(2);
    return ofs.good();
}

// ============================================================
// JsonInputArchive::Impl
// ============================================================

struct JsonInputArchive::Impl {
    // 节点栈：每个元素是指向 json 节点的指针 + 遍历状态
    struct StackEntry {
        const json* node;           // 当前层级的 JSON 节点
        bool isArray = false;
        uint32_t arrayIndex = 0;    // 数组遍历时的当前下标
        uint32_t arraySize = 0;
    };

    json root;                      // 拥有根 JSON 节点
    std::vector<StackEntry> stack;

    bool setRoot(const std::string& content, std::string& error) {
        try {
            root = json::parse(content);
            return true;
        } catch (const json::parse_error& e) {
            error = std::string("JSON parse error: ") + e.what();
            return false;
        }
    }

    const json* current() const {
        return stack.empty() ? nullptr : stack.back().node;
    }

    const json* lookupKey(const json* obj, std::string_view name) const {
        if (!obj || !obj->is_object()) return nullptr;
        auto it = obj->find(name);
        return (it != obj->end()) ? &*it : nullptr;
    }

    const json* nextArrayElement() {
        if (stack.empty()) return nullptr;
        auto& top = stack.back();
        if (!top.isArray) return nullptr;
        if (top.arrayIndex >= top.arraySize) return nullptr;
        return &(*top.node)[top.arrayIndex++];
    }
};

// ============================================================
// JsonInputArchive
// ============================================================

JsonInputArchive::JsonInputArchive(std::string_view jsonStr)
    : m_impl(std::make_unique<Impl>()) {
    std::string error;
    if (!m_impl->setRoot(std::string(jsonStr), error)) {
        setError(error);
    }
}

JsonInputArchive::JsonInputArchive(const std::filesystem::path& filePath)
    : m_impl(std::make_unique<Impl>()) {
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs) {
        setError("Cannot open file: " + filePath.string());
        return;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    std::string error;
    if (!m_impl->setRoot(content, error)) {
        setError(error);
    }
}

JsonInputArchive::~JsonInputArchive() = default;

ArchiveResult JsonInputArchive::beginObject() {
    if (hasError()) return {};

    // 从数组或根节点获取下一个值
    const json* node = nullptr;
    if (m_impl->stack.empty()) {
        node = &m_impl->root;
    } else {
        auto& top = m_impl->stack.back();
        if (top.isArray) {
            node = m_impl->nextArrayElement();
        } else {
            node = top.node;  // key() 已经设置好了
        }
    }

    if (!node || !node->is_object()) {
        auto err = ArchiveError::typeMismatch("object", node ? node->type_name() : "null");
        return tl::make_unexpected(std::move(err));
    }

    m_impl->stack.push_back({node, false, 0, 0});
    return {};
}

ArchiveResult JsonInputArchive::endObject() {
    if (hasError()) return {};
    if (!m_impl->stack.empty()) {
        m_impl->stack.pop_back();
    }
    return {};
}

ArchiveResult JsonInputArchive::key(std::string_view name) {
    if (hasError()) return {};

    if (m_impl->stack.empty()) {
        return tl::make_unexpected(ArchiveError::missingKey(name));
    }

    auto& top = m_impl->stack.back();
    if (top.isArray) {
        return tl::make_unexpected(
            ArchiveError::make(ArchiveError::Code::FormatError,
                               "Cannot call key() inside an array"));
    }

    const json* val = m_impl->lookupKey(top.node, name);
    if (!val) {
        return tl::make_unexpected(ArchiveError::missingKey(name));
    }

    top.node = val;
    return {};
}

ArchiveResult JsonInputArchive::beginArray(uint32_t& outSize) {
    if (hasError()) return {};

    const json* node = nullptr;
    if (m_impl->stack.empty()) {
        node = &m_impl->root;
    } else {
        auto& top = m_impl->stack.back();
        if (top.isArray) {
            node = m_impl->nextArrayElement();
        } else {
            node = top.node;
        }
    }

    if (!node || !node->is_array()) {
        auto err = ArchiveError::typeMismatch("array", node ? node->type_name() : "null");
        return tl::make_unexpected(std::move(err));
    }

    outSize = static_cast<uint32_t>(node->size());
    m_impl->stack.push_back({node, true, 0, outSize});
    return {};
}

ArchiveResult JsonInputArchive::endArray() {
    if (hasError()) return {};
    if (!m_impl->stack.empty()) {
        m_impl->stack.pop_back();
    }
    return {};
}

// --- 原始类型读取 ---

#define MULANGEO_JSON_READ_IMPL(Type, JsonGetter)                                  \
    ArchiveResult JsonInputArchive::read(Type& out) {                              \
        if (hasError()) return {};                                                 \
        const json* node = nullptr;                                                \
        if (!m_impl->stack.empty()) {                                              \
            auto& top = m_impl->stack.back();                                      \
            if (top.isArray) {                                                     \
                node = m_impl->nextArrayElement();                                 \
            } else {                                                               \
                node = top.node;                                                   \
            }                                                                      \
        } else {                                                                   \
            node = &m_impl->root;                                                  \
        }                                                                          \
        if (!node) {                                                               \
            return tl::make_unexpected(ArchiveError::corrupted("No value to read")); \
        }                                                                          \
        try {                                                                      \
            out = node->JsonGetter();                                              \
        } catch (const json::type_error&) {                                        \
            return tl::make_unexpected(                                            \
                ArchiveError::typeMismatch(#Type, node->type_name()));             \
        }                                                                          \
        return {};                                                                 \
    }

MULANGEO_JSON_READ_IMPL(int32_t, get<int32_t>)
MULANGEO_JSON_READ_IMPL(int64_t, get<int64_t>)
MULANGEO_JSON_READ_IMPL(float, get<float>)
MULANGEO_JSON_READ_IMPL(double, get<double>)
MULANGEO_JSON_READ_IMPL(bool, get<bool>)

#undef MULANGEO_JSON_READ_IMPL

ArchiveResult JsonInputArchive::read(std::string& out) {
    if (hasError()) return {};
    const json* node = nullptr;
    if (!m_impl->stack.empty()) {
        auto& top = m_impl->stack.back();
        node = top.isArray ? m_impl->nextArrayElement() : top.node;
    } else {
        node = &m_impl->root;
    }
    if (!node) {
        return tl::make_unexpected(ArchiveError::corrupted("No value to read"));
    }
    try {
        out = node->get<std::string>();
    } catch (const json::type_error&) {
        return tl::make_unexpected(
            ArchiveError::typeMismatch("string", node->type_name()));
    }
    return {};
}

ArchiveResult JsonInputArchive::readBytes(std::vector<byte>& out) {
    if (hasError()) return {};

    const json* node = nullptr;
    if (!m_impl->stack.empty()) {
        auto& top = m_impl->stack.back();
        node = top.isArray ? m_impl->nextArrayElement() : top.node;
    } else {
        node = &m_impl->root;
    }
    if (!node) {
        return tl::make_unexpected(ArchiveError::corrupted("No value to read"));
    }

    // 期望 {"_bytes": "base64string"} 格式
    if (!node->is_object()) {
        return tl::make_unexpected(
            ArchiveError::typeMismatch("bytes object", node->type_name()));
    }

    auto it = node->find("_bytes");
    if (it == node->end()) {
        return tl::make_unexpected(ArchiveError::missingKey("_bytes"));
    }

    std::string encoded;
    try {
        encoded = it->get<std::string>();
    } catch (const json::type_error&) {
        return tl::make_unexpected(
            ArchiveError::typeMismatch("string", it->type_name()));
    }

    // Base64 解码查找表，按 ASCII 值索引
    static const uint8_t kDecodeTable[256] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 0-15
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 16-31
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,62,0xFF,0xFF,0xFF,63,     // 32-47  '+'=43, '/'=47
        52,53,54,55,56,57,58,59,60,61,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,                     // 48-63  '0'-'9'
        0xFF,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,                                      // 64-79  'A'-'O'
        16,17,18,19,20,21,22,23,24,25,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,                      // 80-95  'P'-'Z'
        0xFF,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,                              // 96-111 'a'-'o'
        42,43,44,45,46,47,48,49,50,51,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,                       // 112-127 'p'-'z'
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    };

    if (encoded.size() % 4 != 0) {
        return tl::make_unexpected(ArchiveError::corrupted("Invalid base64 length"));
    }

    size_t padding = 0;
    if (!encoded.empty() && encoded.back() == '=') padding++;
    if (encoded.size() >= 2 && encoded[encoded.size() - 2] == '=') padding++;

    out.clear();
    out.reserve((encoded.size() / 4) * 3 - padding);

    for (size_t i = 0; i < encoded.size(); i += 4) {
        uint8_t a = kDecodeTable[static_cast<uint8_t>(encoded[i])];
        uint8_t b = kDecodeTable[static_cast<uint8_t>(encoded[i + 1])];
        uint8_t c = (encoded[i + 2] == '=') ? 0 : kDecodeTable[static_cast<uint8_t>(encoded[i + 2])];
        uint8_t d = (encoded[i + 3] == '=') ? 0 : kDecodeTable[static_cast<uint8_t>(encoded[i + 3])];

        uint32_t n = (static_cast<uint32_t>(a) << 18) |
                     (static_cast<uint32_t>(b) << 12) |
                     (static_cast<uint32_t>(c) << 6) |
                     static_cast<uint32_t>(d);

        out.push_back(static_cast<byte>((n >> 16) & 0xFF));
        if (encoded[i + 2] != '=') {
            out.push_back(static_cast<byte>((n >> 8) & 0xFF));
        }
        if (encoded[i + 3] != '=') {
            out.push_back(static_cast<byte>(n & 0xFF));
        }
    }

    return {};
}

bool JsonInputArchive::hasKey(std::string_view name) const {
    if (m_impl->stack.empty()) return false;
    auto& top = m_impl->stack.back();
    if (top.isArray) return false;
    return m_impl->lookupKey(top.node, name) != nullptr;
}

ArchiveResult JsonInputArchive::skipValue() {
    // JSON 格式中，key() 定位后不调用 read() 就等于跳过。
    // 数组中需要前进一个下标。
    if (hasError()) return {};
    if (!m_impl->stack.empty()) {
        auto& top = m_impl->stack.back();
        if (top.isArray && top.arrayIndex < top.arraySize) {
            top.arrayIndex++;
        }
    }
    return {};
}

// ============================================================
// InputArchive 路径栈实现
// ============================================================

void InputArchive::pushPathKey(std::string_view name) {
    // 暂时用简单的路径字符串记录，可后续优化
    (void)name;
}

void InputArchive::pushPathIndex(uint32_t index) {
    (void)index;
}

void InputArchive::popPath() {}

std::string InputArchive::currentPath() const {
    return {};
}

} // namespace MulanGeo::Core
