/**
 * @file Macro.h
 * @brief MULANGEO_OBJECT / MULANGEO_PROPERTY 宏定义
 *
 * 使用方式：
 *
 *   class MyGeometry : public MulanGeo::Core::Object {
 *   public:
 *       MULANGEO_OBJECT(MyGeometry, Object)
 *       // ... 自定义字段和方法 ...
 *
 *       void serialize(MulanGeo::Core::OutputArchive& ar) const override { ... }
 *       void serialize(MulanGeo::Core::InputArchive& ar) override { ... }
 *   };
 *
 * MULANGEO_OBJECT 展开：
 *   1. static ClassInfo 静态成员 + classInfo() 实现
 *   2. create() 工厂方法
 *   3. Nifty Counter 自注册到 ObjectFactory
 */
#pragma once

#include "Object.h"
#include "TypeRegistry.h"

// ============================================================
// MULANGEO_OBJECT — 派生类宏
// ============================================================

#define MULANGEO_OBJECT(ClassName, BaseClassName)                                         \
public:                                                                                   \
    static const ::MulanGeo::Core::ClassInfo& staticClassInfo() {                         \
        static const ::MulanGeo::Core::ClassInfo s_info(                                  \
            #ClassName,                                                                   \
            ::MulanGeo::Core::TypeInfo::of<ClassName>(),                                  \
            &BaseClassName::staticClassInfo(),                                             \
            sizeof(ClassName),                                                             \
            false);                                                                        \
        return s_info;                                                                     \
    }                                                                                     \
                                                                                          \
    const ::MulanGeo::Core::ClassInfo& classInfo() const noexcept override {              \
        return staticClassInfo();                                                          \
    }                                                                                     \
                                                                                          \
    std::unique_ptr<::MulanGeo::Core::Object> create() const override {                   \
        return std::make_unique<ClassName>();                                              \
    }                                                                                     \
                                                                                          \
    static std::unique_ptr<::MulanGeo::Core::Object> createStatic() {                     \
        return std::make_unique<ClassName>();                                              \
    }                                                                                     \
private:                                                                                  \
    static bool _autoRegister_##ClassName() {                                             \
        ::MulanGeo::Core::ObjectFactory::instance().registerType(                         \
            #ClassName, &ClassName::createStatic);                                        \
        ::MulanGeo::Core::TypeRegistry::instance().registerClass(                         \
            #ClassName,                                                                   \
            ::MulanGeo::Core::TypeInfo::of<ClassName>(),                                  \
            &BaseClassName::staticClassInfo(),                                             \
            sizeof(ClassName),                                                             \
            false);                                                                        \
        return true;                                                                      \
    }                                                                                     \
    static const bool _registered_##ClassName;                                            \
public:

// 静态成员定义（放在 .cpp 文件中，或者用 MULANGEO_OBJECT_IMPL）
#define MULANGEO_OBJECT_IMPL(ClassName)                                                   \
    const bool ClassName::_registered_##ClassName =                                       \
        ClassName::_autoRegister_##ClassName();

// ============================================================
// MULANGEO_PROPERTY — 属性注册宏
//
// 使用方式（在 .cpp 文件中）：
//   MULANGEO_PROPERTY(MyGeometry, "width", width_, MulanGeo::Core::PropertyAccess::ReadWrite)
// ============================================================

#define MULANGEO_PROPERTY(ClassName, PropName, MemberPtr, Access)                         \
    do {                                                                                  \
        ::MulanGeo::Core::PropertyInfo prop;                                              \
        prop.name = (PropName);                                                           \
        prop.offset = offsetof(ClassName, MemberPtr);                                     \
        prop.access = (Access);                                                           \
        ::MulanGeo::Core::TypeRegistry::instance().registerProperty(#ClassName,           \
                                                                      std::move(prop));   \
    } while (0)
