# App模块 - 主程序入口
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Application.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Core
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
