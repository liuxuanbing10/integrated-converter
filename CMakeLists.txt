cmake_minimum_required(VERSION 3.25)
project(integrated_converter VERSION 1.3.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# Shared source/header lists (also used by tests/CMakeLists.txt)
include(cmake/sources.cmake)
set(SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${CONVERTER_SOURCES}
    ${UI_SOURCES}
    ${CLI_SOURCES}
)
set(HEADERS
    ${CORE_HEADERS}
    ${CONVERTER_HEADERS}
    ${UI_HEADERS}
    ${CLI_HEADERS}
)
set(RESOURCES resources/resources.qrc)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/converters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)
# 不使用 WIN32_EXECUTABLE，以便 CLI 模式下能正常继承父控制台、
# shell 也会同步等待进程结束；GUI 模式下通过 FreeConsole 主动隐藏窗口。
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
# 每次编译后将可执行文件复制到项目根目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.exe"
    COMMENT "复制可执行文件到项目根目录"
)
option(BUILD_TESTS "Build test suite" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
