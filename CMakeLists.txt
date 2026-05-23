cmake_minimum_required(VERSION 3.16)
project(integrated_converter VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
set(CORE_SOURCES
    src/core/conversion_task.cpp
    src/core/task_manager.cpp
    src/core/task_runnable.cpp
    src/core/config_manager.cpp
    src/core/logger.cpp
    src/core/error_types.cpp
    src/core/error_handler.cpp
    src/core/retry_manager.cpp
    src/core/skill_manager.cpp
    src/core/memory_monitor.cpp
    src/core/large_file_handler.cpp
)
set(CORE_HEADERS
    src/core/iconverter.h
    src/core/conversion_task.h
    src/core/task_manager.h
    src/core/task_runnable.h
    src/core/config_manager.h
    src/core/logger.h
    src/core/error_types.h
    src/core/error_handler.h
    src/core/retry_manager.h
    src/core/skill_manager.h
    src/core/memory_monitor.h
    src/core/large_file_handler.h
)
set(CONVERTER_SOURCES
    src/converters/ffmpeg_converter.cpp
    src/converters/pandoc_converter.cpp
    src/converters/segmented_converter.cpp
)
set(CONVERTER_HEADERS
    src/converters/ffmpeg_converter.h
    src/converters/pandoc_converter.h
    src/converters/segmented_converter.h
)
set(UI_SOURCES
    src/ui/main_window.cpp
    src/ui/task_list_widget.cpp
    src/ui/config_panel.cpp
    src/ui/file_list_widget.cpp
    src/ui/progress_widget.cpp
    src/ui/batch_conversion_summary.cpp
    src/ui/batch_convert_dialog.cpp
    src/ui/error_dialog.cpp
    src/ui/skill_invoke_dialog.cpp
)
set(UI_HEADERS
    src/ui/main_window.h
    src/ui/task_list_widget.h
    src/ui/config_panel.h
    src/ui/file_list_widget.h
    src/ui/progress_widget.h
    src/ui/batch_conversion_summary.h
    src/ui/batch_convert_dialog.h
    src/ui/error_dialog.h
    src/ui/skill_invoke_dialog.h
)
set(SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${CONVERTER_SOURCES}
    ${UI_SOURCES}
)
set(HEADERS
    ${CORE_HEADERS}
    ${CONVERTER_HEADERS}
    ${UI_HEADERS}
)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/converters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()
option(BUILD_TESTS "Build test suite" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
