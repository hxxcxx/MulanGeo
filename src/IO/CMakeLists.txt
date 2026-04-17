# IO - CAD 文件导入库 (DLL)
add_library(IO SHARED
    ImporterFactory.cpp
    OCCTImporter.cpp
    ImageLoader.cpp
)

# IO 的 MeshNode 继承 Engine 的 SceneNode
target_link_libraries(IO PUBLIC Engine)

# 构建时将头文件复制到 build/include/MulanGeo/IO/...
file(GLOB_RECURSE IO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
foreach(hdr ${IO_HEADERS})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${hdr}")
    configure_file("${hdr}" "${CMAKE_BINARY_DIR}/include/MulanGeo/IO/${rel}" COPYONLY)
endforeach()

target_include_directories(IO PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_include_directories(IO PRIVATE
    ${OCCT_INCLUDE_DIR}
)

# 链接 OCCT 库
foreach(lib ${OCCT_LIBS})
    find_library(OCCT_${lib} ${lib} PATHS ${OCCT_LIB_DIR} NO_DEFAULT_PATH)
    if(OCCT_${lib})
        target_link_libraries(IO PRIVATE ${OCCT_${lib}})
    else()
        message(WARNING "OCCT library not found: ${lib}")
    endif()
endforeach()

target_compile_definitions(IO PRIVATE
    IO_EXPORTS
    BUILDING_IO
    $<$<CONFIG:Debug>:_DEBUG>
)

# --- stb (图像加载) ---
find_path(STB_INCLUDE_DIRS "stb_image.h")
if(STB_INCLUDE_DIRS)
    target_include_directories(IO PRIVATE ${STB_INCLUDE_DIRS})
else()
    message(WARNING "stb headers not found — ImageLoader will fail to compile")
endif()
