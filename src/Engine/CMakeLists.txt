# Engine - 渲染后端与场景管理（静态库）
add_library(Engine STATIC
    RHI/vertex_layout_test.cpp
    RHI/DeviceFactory.cpp
    Math/Mat4.cpp
    RHI/Vulkan/VKBuffer.cpp
    RHI/Vulkan/VKTexture.cpp
    RHI/Vulkan/VKShader.cpp
    RHI/Vulkan/VKFence.cpp
    RHI/Vulkan/VKFrameContext.cpp
    RHI/Vulkan/VKCommandList.cpp
    RHI/Vulkan/VKPipelineState.cpp
    RHI/Vulkan/VKDescriptorAllocator.cpp
    RHI/Vulkan/VKUploadContext.cpp
    RHI/Vulkan/VKSwapChain.cpp
    RHI/Vulkan/VKDevice.cpp
    RHI/OpenGL/GLDevice.cpp
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

# --- OpenGL + GLAD (vcpkg) ---
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)

# --- Vulkan ---
find_package(Vulkan REQUIRED)

# VMA (vulkan-memory-allocator)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_link_libraries(Engine PUBLIC
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    OpenGL::GL
    glad::glad
)

target_compile_definitions(Engine PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

# MSVC: 多文件并行编译写同一 PDB 需要 /FS
if(MSVC)
    target_compile_options(Engine PRIVATE /FS)
endif()

# --- Shader 编译 (HLSL → SPIR-V via dxc) ---
# 查找 dxc
find_program(DXC_EXECUTABLE dxc
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "${Vulkan_INSTALL_DIR}/Bin"
        "C:/VulkanSDK/*/Bin"
)

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

if(DXC_EXECUTABLE)
    message(STATUS "Found dxc: ${DXC_EXECUTABLE}")

    set(SHADER_SOURCES
        solid.vert.hlsl
        solid.frag.hlsl
        wireframe.vert.hlsl
        wireframe.frag.hlsl
        pick.vert.hlsl
        pick.frag.hlsl
    )

    # Common.hlsli 作为依赖（不单独编译）
    set(SHADER_COMMON "${SHADER_SOURCE_DIR}/Common.hlsli")

    # 确定每个 shader 文件的 stage 和入口点
    set(SOLID_VERT_STAGE    vs)
    set(SOLID_VERT_ENTRY    main)
    set(SOLID_FRAG_STAGE    ps)
    set(SOLID_FRAG_ENTRY    main)
    set(WIREFRAME_VERT_STAGE vs)
    set(WIREFRAME_VERT_ENTRY main)
    set(WIREFRAME_FRAG_STAGE ps)
    set(WIREFRAME_FRAG_ENTRY main)
    set(PICK_VERT_STAGE     vs)
    set(PICK_VERT_ENTRY     main)
    set(PICK_FRAG_STAGE     ps)
    set(PICK_FRAG_ENTRY     main)

    foreach(SHADER_FILE ${SHADER_SOURCES})
        # 转为大写标识符用于变量查找
        string(REPLACE "." "_" SHADER_ID "${SHADER_FILE}")
        string(TOUPPER "${SHADER_ID}" SHADER_ID_UPPER)

        # 获取 stage 和 entry
        set(SHADER_STAGE ${${SHADER_ID_UPPER}_STAGE})
        set(SHADER_ENTRY ${${SHADER_ID_UPPER}_ENTRY})

        if(NOT SHADER_STAGE)
            # 从文件名推断
            if("${SHADER_FILE}" MATCHES "\\.vert\\.")
                set(SHADER_STAGE vs)
            elseif("${SHADER_FILE}" MATCHES "\\.frag\\.")
                set(SHADER_STAGE ps)
            endif()
            set(SHADER_ENTRY main)
        endif()

        # 输出文件名: solid.vert.hlsl → solid.vert.spv
        string(REPLACE ".hlsl" ".spv" SPV_FILE "${SHADER_FILE}")

        set(SPV_INPUT  "${SHADER_SOURCE_DIR}/${SHADER_FILE}")
        set(SPV_OUTPUT "${SHADER_OUTPUT_DIR}/${SPV_FILE}")

        add_custom_command(
            OUTPUT  "${SPV_OUTPUT}"
            COMMAND "${DXC_EXECUTABLE}"
                -T ${SHADER_STAGE}_6_0
                -E ${SHADER_ENTRY}
                -spirv
                -I "${SHADER_SOURCE_DIR}"
                -Fo "${SPV_OUTPUT}"
                "${SPV_INPUT}"
            DEPENDS "${SPV_INPUT}" "${SHADER_COMMON}"
            COMMENT "Compiling shader: ${SHADER_FILE} → ${SPV_FILE}"
        )

        list(APPEND SPIRV_FILES "${SPV_OUTPUT}")
    endforeach()

    add_custom_target(Shaders ALL DEPENDS ${SPIRV_FILES})
    add_dependencies(Engine Shaders)

    # 将 shader 输出目录作为 include path（方便运行时加载）
    target_compile_definitions(Engine PRIVATE
        SHADER_DIR="${SHADER_OUTPUT_DIR}"
    )
else()
    message(WARNING "dxc not found — shaders will not be compiled. Set VULKAN_SDK or install dxc.")
endif()
