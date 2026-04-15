# Engine - 渲染后端与场景管理（静态库 — 主要是头文件）
add_library(Engine STATIC
    RHI/vertex_layout_test.cpp
    Math/Mat4.cpp
)

# 构建时将头文件复制到 build/include/MulanGeo/Engine/...
file(GLOB_RECURSE ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
foreach(hdr ${ENGINE_HEADERS})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${hdr}")
    configure_file("${hdr}" "${CMAKE_BINARY_DIR}/include/MulanGeo/Engine/${rel}" COPYONLY)
endforeach()

target_include_directories(Engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(Engine PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)
