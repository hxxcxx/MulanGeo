# IO Module - CAD file import library (DLL)
add_library(IO SHARED
    src/ImporterFactory.cpp
    src/OCCTImporter.cpp
)

target_include_directories(IO PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

# Copy public headers to build/include
file(GLOB_RECURSE IO_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
foreach(hdr ${IO_HEADERS})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/include" "${hdr}")
    configure_file("${hdr}" "${CMAKE_BINARY_DIR}/include/${rel}" COPYONLY)
endforeach()

target_include_directories(IO PRIVATE
    ${OCCT_INCLUDE_DIR}
)

# Link OCCT libraries (find .lib files)
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
