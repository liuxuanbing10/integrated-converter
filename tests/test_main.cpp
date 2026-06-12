#include <QTest>
#include <QCoreApplication>
#include <cstdlib>
#include "core/test_logger.h"
#include "core/test_config_manager.h"
#include "core/test_task_manager.h"
#include "core/test_error_handler.h"
#include "core/test_error_types.h"
#include "core/memory_monitor.h"
#include "core/task_manager.h"
#include "core/ilogger.h"
#include "core/logger.h"
#include "converters/test_ffmpeg_converter.h"
#include "converters/test_ffmpeg_progress_parser.h"
#include "converters/test_pandoc_converter.h"
#include "converters/test_imagemagick_converter.h"
#include "converters/test_segmented_converter.h"
#include "core/test_large_file_handler.h"
#include "core/test_memory_monitor.h"
#include "core/test_retry_manager.h"
#include "core/test_skill_manager.h"
#include "core/test_format_registry.h"

int main(int argc, char *argv[]) {
    int status = 0;
    {
        QCoreApplication app(argc, argv);
        Logger appLogger;
        g_logger = &appLogger;
        appLogger.setConsoleOutput(false);
        appLogger.setFileOutput(false);
        TestLogger testLogger;
        status |= QTest::qExec(&testLogger, argc, argv);
        TestConfigManager testConfig;
        status |= QTest::qExec(&testConfig, argc, argv);
        TestTaskManager testTaskManager;
        status |= QTest::qExec(&testTaskManager, argc, argv);
        TestErrorHandler testErrorHandler;
        status |= QTest::qExec(&testErrorHandler, argc, argv);
        TestErrorTypes testErrorTypes;
        status |= QTest::qExec(&testErrorTypes, argc, argv);
        TestFFmpegConverter testFFmpeg;
        status |= QTest::qExec(&testFFmpeg, argc, argv);
        TestFfmpegProgressParser testParser;
        status |= QTest::qExec(&testParser, argc, argv);
        TestPandocConverter testPandoc;
        status |= QTest::qExec(&testPandoc, argc, argv);
        TestImageMagickConverter testImageMagick;
        status |= QTest::qExec(&testImageMagick, argc, argv);
        TestSegmentedConverter testSegmented;
        status |= QTest::qExec(&testSegmented, argc, argv);
        TestLargeFileHandler testLargeFileHandler;
        status |= QTest::qExec(&testLargeFileHandler, argc, argv);
        TestMemoryMonitor testMemoryMonitor;
        status |= QTest::qExec(&testMemoryMonitor, argc, argv);
        TestRetryManager testRetryManager;
        status |= QTest::qExec(&testRetryManager, argc, argv);
        TestSkillManager testSkillManager;
        status |= QTest::qExec(&testSkillManager, argc, argv);
        TestFormatRegistry testFormatRegistry;
        status |= QTest::qExec(&testFormatRegistry, argc, argv);
        // Clean shutdown of singletons in reverse dependency order to prevent
        // hangs during static destruction (TaskManager thread pool, MemoryMonitor timer)
        TaskManager::instance()->cancelAllTasks();
        MemoryMonitor::instance()->stopMonitoring();
        appLogger.setFileOutput(false);
    }
    // _Exit skips static destruction which can hang due to singleton dependency order
    // (TaskManager, MemoryMonitor, etc.). Exit code is preserved intact.
    _Exit(status);
}
