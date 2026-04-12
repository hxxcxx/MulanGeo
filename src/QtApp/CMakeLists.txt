# QtApp Module - Main application
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME}
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    IO
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ── Post-build: copy all OCCT DLLs ──
file(GLOB OCCT_DLLS "${OCCT_ROOT}/win64/vc14/bin/*.dll")
foreach(dll ${OCCT_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${dll}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()

# ── Post-build: copy OCCT 3rdparty DLLs ──
file(GLOB_RECURSE OCCT_3RDPARTY_DLLS "F:/dev/3rdparty-vc14-64/*.dll")
foreach(dll ${OCCT_3RDPARTY_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${dll}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()

# ── Post-build: deploy Qt DLLs via windeployqt ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
        --no-translations --no-opengl-sw --no-system-d3d-compiler
        "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Deploying Qt DLLs via windeployqt..."
)
