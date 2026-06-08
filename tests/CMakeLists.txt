cmake_minimum_required(VERSION 3.16)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Test)

# Shared source/header lists from root (must come before target creation)
include(../cmake/sources.cmake)

set(TEST_SOURCES
    test_main.cpp

    core/test_logger.cpp
    core/test_config_manager.cpp
    core/test_task_manager.cpp
    core/test_error_handler.cpp
    core/test_error_types.cpp
    converters/test_ffmpeg_converter.cpp
    converters/test_ffmpeg_progress_parser.cpp
    converters/test_pandoc_converter.cpp
    converters/test_imagemagick_converter.cpp
)
add_executable(test_runner ${TEST_SOURCES} ${CORE_SOURCES} ${CONVERTER_SOURCES})
target_include_directories(test_runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/converters
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(test_runner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Test
)
if(WIN32)
    set_target_properties(test_runner PROPERTIES
        WIN32_EXECUTABLE OFF
    )
endif()
set_target_properties(test_runner PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)
enable_testing()
add_test(NAME UnitTests COMMAND test_runner)

