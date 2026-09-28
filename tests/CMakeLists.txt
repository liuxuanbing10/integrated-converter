cmake_minimum_required(VERSION 3.25)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

# Shared source/header lists from root (must come before target creation)
include(../cmake/sources.cmake)

set(TEST_SOURCES
    test_main.cpp

    cli/test_cli_runner.cpp

    core/test_logger.cpp
    core/test_config_manager.cpp
    core/test_task_manager.cpp
    core/test_error_types.cpp
    core/test_conversion_coordinator.cpp
    core/test_conversion_planner.cpp
    core/test_portable_mode.cpp
    converters/test_ffmpeg_converter.cpp
    converters/test_ffmpeg_progress_parser.cpp
    converters/test_pandoc_converter.cpp
    converters/test_imagemagick_converter.cpp

    core/test_large_file_handler.cpp
    core/test_format_registry.cpp

    ui/test_main_window.cpp
    ui/test_params_widgets.cpp
)
set(WIDGET_SOURCES
    ${PROJECT_SOURCE_DIR}/src/ui/theme.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/image_params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/document_params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/audio_params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/video_params_widget.cpp
    # §零.2 harness: MainWindow must link into the test binary so the
    # Qt-native grab取证 test can pin pre/post-refactor visual parity.
    ${PROJECT_SOURCE_DIR}/src/ui/main_window.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/settings_page.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/window_drop.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/task_list_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/file_category_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/progress_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/batch_conversion_summary.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/conversion_params_dialog.cpp
    ${PROJECT_SOURCE_DIR}/resources/resources.qrc
)
add_executable(test_runner ${TEST_SOURCES} ${WIDGET_SOURCES} ${CORE_SOURCES} ${CONVERTER_SOURCES} ${CLI_SOURCES})
target_include_directories(test_runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/converters
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(test_runner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
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
# UI tests assert the single-version-source contract (§零.5), so the
# CMake-injected APP_VERSION must exist in the test target too.
target_compile_definitions(test_runner PRIVATE APP_VERSION="${PROJECT_VERSION}")
enable_testing()
add_test(NAME UnitTests COMMAND test_runner)

