/**
 * Result<T> 使用示例 — 演示 Ok / Err / PROPAGATE / TRY / CORE_ERR
 *
 * 编译: cl /std:c++20 /EHsc /I<include_path> result_demo.cpp
 */

#include "Core/Result/Result.h"
#include <cstdio>
#include <string>
#include <vector>

// ============================================================
// 1. 基本用法 — 返回值或错误
// ============================================================

MulanGeo::Core::Result<int> parseAge(const std::string& input) {
    if (input.empty()) {
        return MulanGeo::Core::Err<int>(CORE_ERR("age is empty"));
        // Error 自动捕获当前文件名 + 行号
    }
    try {
        int age = std::stoi(input);
        if (age < 0 || age > 200) {
            return MulanGeo::Core::Err<int>(
                CORE_ERR_CODE(1001, "age out of range: %d", age));
        }
        return Ok(age);  // Ok() 自动推导返回类型
    } catch (...) {
        return MulanGeo::Core::Err<int>(CORE_ERR("not a valid number"));
    }
}

// ============================================================
// 2. Result<void> — 只关心成功/失败
// ============================================================

MulanGeo::Core::Result<void> validateName(const std::string& name) {
    if (name.size() < 2) {
        return MulanGeo::Core::Err<void>(CORE_ERR("name too short"));
    }
    if (name.size() > 50) {
        return MulanGeo::Core::Err<void>(CORE_ERR("name too long"));
    }
    return Ok();  // void 成功
}

// ============================================================
// 3. PROPAGATE — 链式调用，错误自动向上传播
// ============================================================

MulanGeo::Core::Result<std::string> buildGreeting(const std::string& nameInput,
                                                    const std::string& ageInput) {
    // PROPAGATE: 失败时直接 return 错误，成功时绑定值到 name_ok
    PROPAGATE(name_ok, validateName(nameInput));

    // PROPAGATE: 解析年龄
    PROPAGATE(age_ok, parseAge(ageInput));

    // 正常逻辑
    return Ok("Hello " + *name_ok + ", you are " + std::to_string(*age_ok) + " years old!");
}

// ============================================================
// 4. TRY — 不需要绑定值的错误传播
// ============================================================

MulanGeo::Core::Result<void> registerUser(const std::string& name,
                                            const std::string& age) {
    TRY(validateName(name));          // 失败直接 return，不需要变量名
    TRY(parseAge(age));               // 同上，值丢弃只检查成功/失败

    printf("[OK] User '%s' registered.\n", name.c_str());
    return Ok();
}

// ============================================================
// 5. 深层调用链 — 错误逐层传播，保留原始位置
// ============================================================

MulanGeo::Core::Result<double> computeScore(const std::string& ageStr) {
    PROPAGATE(age, parseAge(ageStr));
    return Ok(100.0 / *age);
}

MulanGeo::Core::Result<std::string> formatReport(const std::string& ageStr) {
    PROPAGATE(score, computeScore(ageStr));
    return Ok("Score: " + std::to_string(*score));
}

// ============================================================
// main — 演示各种场景
// ============================================================

using MulanGeo::Core::Error;

void printResult(const char* label, const MulanGeo::Core::Result<std::string>& r) {
    if (r) {
        printf("[PASS] %s => %s\n", label, r->c_str());
    } else {
        const Error& e = r.error();
        printf("[FAIL] %s => code=%d msg='%s' @ %s:%u\n",
               label, e.code, e.message.c_str(),
               e.file ? e.file : "(null)", e.line);
    }
}

void printVoid(const char* label, const MulanGeo::Core::Result<void>& r) {
    if (r) {
        printf("[PASS] %s => ok\n", label);
    } else {
        const Error& e = r.error();
        printf("[FAIL] %s => code=%d msg='%s' @ %s:%u\n",
               label, e.code, e.message.c_str(),
               e.file ? e.file : "(null)", e.line);
    }
}

int main() {
    printf("=== Result<T> Demo ===\n\n");

    // 1. 基本成功/失败
    auto r1 = parseAge("25");
    printf("[parseAge] 25 => %s\n", r1 ? "ok" : r1.error().message.c_str());

    auto r2 = parseAge("abc");
    printf("[parseAge] abc => %s\n", r2 ? "ok" : r2.error().message.c_str());

    auto r3 = parseAge("999");
    printf("[parseAge] 999 => %s (code=%d)\n", r3 ? "ok" : r3.error().message.c_str(), r3.error().code);

    printf("\n");

    // 2. PROPAGATE 链式
    printResult("greeting(Alice, 25)", buildGreeting("Alice", "25"));
    printResult("greeting(A, 25)",     buildGreeting("A", "25"));        // name too short
    printResult("greeting(Alice, abc)", buildGreeting("Alice", "abc"));  // parse fail
    printResult("greeting(Alice, -1)",  buildGreeting("Alice", "-1"));   // out of range

    printf("\n");

    // 3. TRY (void)
    printVoid("register(Bob, 30)",   registerUser("Bob", "30"));
    printVoid("register(X, 30)",     registerUser("X", "30"));
    printVoid("register(Bob, hello)", registerUser("Bob", "hello"));

    printf("\n");

    // 4. 深层传播 — 错误保留最原始的 source_location
    printResult("formatReport(30)",  formatReport("30"));
    printResult("formatReport(0)",   formatReport("0"));    // parseAge ok, computeScore ok (div by zero = inf)
    printResult("formatReport(bad)", formatReport("bad"));  // 错误来自 parseAge 内部

    return 0;
}
