cmake_minimum_required(VERSION 3.16)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Test)
set(TEST_SOURCES
    test_main.cpp
    test_integration.cpp
    test_performance.cpp
    core/test_logger.cpp
    core/test_config_manager.cpp
    core/test_task_manager.cpp
    core/test_error_handler.cpp
    converters/test_ffmpeg_converter.cpp
    converters/test_pandoc_converter.cpp
    converters/test_imagemagick_converter.cpp
)
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/conversion_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/task_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/task_runnable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/config_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/error_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/error_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/retry_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/skill_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/memory_monitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/large_file_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/format_registry.cpp
)
set(CONVERTER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/converters/ffmpeg_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/converters/pandoc_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/converters/segmented_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/converters/imagemagick_converter.cpp
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
