# IO模块 - CAD文件读取 (OCCT集成)
add_library(IO STATIC
    src/CADImporter.cpp
)

if(OpenCASCADE_FOUND)
    target_compile_definitions(IO PUBLIC -DHAVE_OCC)
    target_link_libraries(IO PUBLIC
        ${OCC_LIBRARY}
        TKernel TKMath TKBRep TKTopAlg TKPrim TKBO TKShHealing TKMesh TKOffset
    )
endif()

target_include_directories(IO PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(IO PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
