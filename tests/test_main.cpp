#include <QTest>
#include <QCoreApplication>
#include <cstdlib>
#include "core/test_logger.h"
#include "core/test_config_manager.h"
#include "core/test_task_manager.h"
#include "core/test_error_handler.h"
#include "core/memory_monitor.h"
#include "core/task_manager.h"
#include "core/logger.h"
#include "converters/test_ffmpeg_converter.h"
#include "converters/test_pandoc_converter.h"
#include "converters/test_imagemagick_converter.h"

int main(int argc, char *argv[]) {
    int status = 0;
    {
        QCoreApplication app(argc, argv);
        TestLogger testLogger;
        status |= QTest::qExec(&testLogger, argc, argv);
        TestConfigManager testConfig;
        status |= QTest::qExec(&testConfig, argc, argv);
        TestTaskManager testTaskManager;
        status |= QTest::qExec(&testTaskManager, argc, argv);
        TestErrorHandler testErrorHandler;
        status |= QTest::qExec(&testErrorHandler, argc, argv);
        TestFFmpegConverter testFFmpeg;
        status |= QTest::qExec(&testFFmpeg, argc, argv);
        TestPandocConverter testPandoc;
        status |= QTest::qExec(&testPandoc, argc, argv);
        TestImageMagickConverter testImageMagick;
        status |= QTest::qExec(&testImageMagick, argc, argv);
        // Clean shutdown of singletons in reverse dependency order to prevent
        // hangs during static destruction (TaskManager thread pool, MemoryMonitor timer)
        TaskManager::instance()->cancelAllTasks();
        MemoryMonitor::instance()->stopMonitoring();
        Logger::instance().setFileOutput(false);
    }
    // _Exit skips static destruction which can hang due to singleton dependency order
    // (TaskManager, MemoryMonitor, etc.). Exit code is preserved intact.
    _Exit(status);
}
