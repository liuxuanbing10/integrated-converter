#include <QTest>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <cstdlib>
#include <cstdio>
#include "core/test_logger.h"
#include "core/test_config_manager.h"
#include "core/test_task_manager.h"
#include "core/test_error_types.h"
#include "core/task_manager.h"
#include "core/logger.h"
#include "converters/test_ffmpeg_converter.h"
#include "converters/test_ffmpeg_progress_parser.h"
#include "converters/test_pandoc_converter.h"
#include "converters/test_imagemagick_converter.h"
#include "converters/test_segmented_converter.h"
#include "core/test_large_file_handler.h"
#include "core/test_format_registry.h"

// Run a test suite and log result to file via QFile (bypasses stdio buffering)
static int runSuite(QObject *test, int argc, char **argv,
                    const QString &name, QTextStream &log) {
    log << "=== " << name << " ===\n";
    log.flush();
    int result = QTest::qExec(test, argc, argv);
    log << "=== " << name << ": "
        << (result == 0 ? "PASS" : "FAIL")
        << " (exit=" << result << ") ===\n\n";
    log.flush();
    return result;
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    int status = 0;

    // Open log file via QFile — bypasses stdio buffering entirely
    QFile logFile("test_results.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream log(&logFile);

        log << "test_runner.exe started\n";
        log.flush();

        {
            QCoreApplication app(argc, argv);
            Logger appLogger;
            g_logger = &appLogger;
            appLogger.setConsoleOutput(false);
            appLogger.setFileOutput(false);

            log << "QCoreApplication created, running test suites...\n";
            log.flush();

            TestLogger testLogger;
            status |= runSuite(&testLogger, argc, argv, "TestLogger", log);
            TestConfigManager testConfig;
            status |= runSuite(&testConfig, argc, argv, "TestConfigManager", log);
            TestTaskManager testTaskManager;
            status |= runSuite(&testTaskManager, argc, argv, "TestTaskManager", log);
            TestErrorTypes testErrorTypes;
            status |= runSuite(&testErrorTypes, argc, argv, "TestErrorTypes", log);
            TestFFmpegConverter testFFmpeg;
            status |= runSuite(&testFFmpeg, argc, argv, "TestFFmpegConverter", log);
            TestFfmpegProgressParser testParser;
            status |= runSuite(&testParser, argc, argv, "TestFfmpegProgressParser", log);
            TestPandocConverter testPandoc;
            status |= runSuite(&testPandoc, argc, argv, "TestPandocConverter", log);
            TestImageMagickConverter testImageMagick;
            status |= runSuite(&testImageMagick, argc, argv, "TestImageMagickConverter", log);
            TestSegmentedConverter testSegmented;
            status |= runSuite(&testSegmented, argc, argv, "TestSegmentedConverter", log);
            TestLargeFileHandler testLargeFileHandler;
            status |= runSuite(&testLargeFileHandler, argc, argv, "TestLargeFileHandler", log);
            TestFormatRegistry testFormatRegistry;
            status |= runSuite(&testFormatRegistry, argc, argv, "TestFormatRegistry", log);

            // Clean shutdown of singletons in reverse dependency order to prevent
            // hangs during static destruction (TaskManager thread pool)
            TaskManager::instance()->cancelAllTasks();
            appLogger.setFileOutput(false);

            log << "FINAL STATUS: " << status
                << " (" << (status == 0 ? "ALL PASS" : "SOME FAILURES") << ")\n";
            log.flush();
        }
        logFile.close();
    }

    // _Exit skips static destruction which can hang due to singleton dependency order
    // (TaskManager, etc.). Exit code is preserved intact.
    std::fflush(stdout);
    _Exit(status);
}
