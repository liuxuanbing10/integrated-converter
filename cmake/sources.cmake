# ── Shared source/header lists for integrated_converter ────────────────
# Included from both top-level CMakeLists.txt and tests/CMakeLists.txt.
# Uses ${PROJECT_SOURCE_DIR} as the path root.
# ============================================================================

# ── Core framework ──────────────────────────────────────────────────────────
set(CORE_SOURCES
    ${PROJECT_SOURCE_DIR}/src/core/conversion_task.cpp
    ${PROJECT_SOURCE_DIR}/src/core/task_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/task_runnable.cpp
    ${PROJECT_SOURCE_DIR}/src/core/config_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/logger.cpp
    ${PROJECT_SOURCE_DIR}/src/core/error_types.cpp
    ${PROJECT_SOURCE_DIR}/src/core/error_handler.cpp
    ${PROJECT_SOURCE_DIR}/src/core/retry_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/skill_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/memory_monitor.cpp
    ${PROJECT_SOURCE_DIR}/src/core/large_file_handler.cpp
    ${PROJECT_SOURCE_DIR}/src/core/format_registry.cpp
)
set(CORE_HEADERS
    ${PROJECT_SOURCE_DIR}/src/core/iconverter.h
    ${PROJECT_SOURCE_DIR}/src/core/conversion_task.h
    ${PROJECT_SOURCE_DIR}/src/core/task_manager.h
    ${PROJECT_SOURCE_DIR}/src/core/task_runnable.h
    ${PROJECT_SOURCE_DIR}/src/core/config_manager.h
    ${PROJECT_SOURCE_DIR}/src/core/ilogger.h
    ${PROJECT_SOURCE_DIR}/src/core/logger.h
    ${PROJECT_SOURCE_DIR}/src/core/error_types.h
    ${PROJECT_SOURCE_DIR}/src/core/error_handler.h
    ${PROJECT_SOURCE_DIR}/src/core/retry_manager.h
    ${PROJECT_SOURCE_DIR}/src/core/skill_manager.h
    ${PROJECT_SOURCE_DIR}/src/core/memory_monitor.h
    ${PROJECT_SOURCE_DIR}/src/core/large_file_handler.h
    ${PROJECT_SOURCE_DIR}/src/core/format_registry.h
)

# ── Converter wrappers ─────────────────────────────────────────────────────
set(CONVERTER_SOURCES
    ${PROJECT_SOURCE_DIR}/src/converters/ffmpeg_converter.cpp
    ${PROJECT_SOURCE_DIR}/src/converters/ffmpeg_progress_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/converters/pandoc_converter.cpp
    ${PROJECT_SOURCE_DIR}/src/converters/segmented_converter.cpp
    ${PROJECT_SOURCE_DIR}/src/converters/imagemagick_converter.cpp
)
set(CONVERTER_HEADERS
    ${PROJECT_SOURCE_DIR}/src/converters/ffmpeg_converter.h
    ${PROJECT_SOURCE_DIR}/src/converters/ffmpeg_progress_parser.h
    ${PROJECT_SOURCE_DIR}/src/converters/pandoc_converter.h
    ${PROJECT_SOURCE_DIR}/src/converters/segmented_converter.h
    ${PROJECT_SOURCE_DIR}/src/converters/imagemagick_converter.h
)

# ── UI sources ─────────────────────────────────────────────────────────────
set(UI_SOURCES
    ${PROJECT_SOURCE_DIR}/src/ui/main_window.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/task_list_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/file_list_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/file_category_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/progress_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/batch_conversion_summary.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/batch_convert_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/error_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/skill_invoke_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/conversion_params_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/image_params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/document_params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/audio_params_widget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/video_params_widget.cpp
)
set(UI_HEADERS
    ${PROJECT_SOURCE_DIR}/src/ui/main_window.h
    ${PROJECT_SOURCE_DIR}/src/ui/task_list_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/file_list_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/file_category_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/progress_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/batch_conversion_summary.h
    ${PROJECT_SOURCE_DIR}/src/ui/batch_convert_dialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/error_dialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/skill_invoke_dialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/conversion_params_dialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/image_params_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/document_params_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/audio_params_widget.h
    ${PROJECT_SOURCE_DIR}/src/ui/video_params_widget.h
)

# ── CLI sources ────────────────────────────────────────────────────────────
set(CLI_SOURCES
    ${PROJECT_SOURCE_DIR}/src/cli/cli_runner.cpp
)
set(CLI_HEADERS
    ${PROJECT_SOURCE_DIR}/src/cli/cli_runner.h
)
