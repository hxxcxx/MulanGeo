# Engine Module - Rendering backend & scene management (DLL)
add_library(Engine SHARED
    src/RHI/vertex_layout_test.cpp
)

target_include_directories(Engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

# Copy public headers to build/include
file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
foreach(hdr ${ENGINE_HEADERS})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/include" "${hdr}")
    configure_file("${hdr}" "${CMAKE_BINARY_DIR}/include/${rel}" COPYONLY)
endforeach()

target_compile_definitions(Engine PRIVATE
    ENGINE_EXPORTS
    BUILDING_ENGINE
    $<$<CONFIG:Debug>:_DEBUG>
)
