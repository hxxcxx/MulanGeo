# Core - 基础设施层（动态库）
#
# 图片加载/保存、文件 IO、日志等通用功能。
# 仅依赖 stb，所有模块均可引入。
add_library(Core SHARED
    Image/Image.cpp
)

# 构建时将头文件复制到 build/include/MulanGeo/Core/...
file(GLOB_RECURSE CORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/**/*.h"
)
foreach(hdr ${CORE_HEADERS})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${hdr}")
    configure_file("${hdr}" "${CMAKE_BINARY_DIR}/include/MulanGeo/Core/${rel}" COPYONLY)
endforeach()

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

# --- stb (图像加载) ---
find_path(STB_INCLUDE_DIRS "stb_image.h")
if(STB_INCLUDE_DIRS)
    target_include_directories(Core PRIVATE ${STB_INCLUDE_DIRS})
else()
    message(WARNING "stb headers not found — Image.cpp will fail to compile")
endif()

# CMake 对 SHARED 库自动定义 <name>_EXPORTS 宏（如 Core_EXPORTS）
# 我们用 Core_EXPORTS 来触发 dllexport
target_compile_definitions(Core PRIVATE
    BUILDING_CORE=1
    $<$<CONFIG:Debug>:_DEBUG>
)

if(MSVC)
    target_compile_options(Core PRIVATE /FS)
endif()
