#include "cli/test_cli_runner.h"
#include "converters/test_ffmpeg_converter.h"
#include "converters/test_ffmpeg_progress_parser.h"
#include "converters/test_imagemagick_converter.h"
#include "converters/test_pandoc_converter.h"
#include "core/logger.h"
#include "core/task_manager.h"
#include "core/test_config_manager.h"
#include "core/test_conversion_coordinator.h"
#include "core/test_conversion_planner.h"
#include "core/test_error_types.h"
#include "core/test_format_registry.h"
#include "core/test_large_file_handler.h"
#include "core/test_logger.h"
#include "core/test_portable_mode.h"
#include "core/test_task_manager.h"
#include "ui/test_main_window.h"
#include "ui/test_params_widgets.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QTest>
#include <QTextStream>

#include <cstdio>
#include <cstdlib>

// Run a test suite and log result to file via QFile (bypasses stdio buffering).
// QTest's own per-function output is redirected with -o into a sibling file:
// on CI runners stdout can be lost entirely, and QTEST_LOGFILE proved
// unreliable on some Qt 6.12 builds.
static int runSuite(QObject* test, int argc, char** argv, const QString& name, QTextStream& log)
{
    log << "=== " << name << " ===\n";
    log.flush();
    QByteArray outArg("-o");
    QByteArray outFile = (name + QStringLiteral(".qtest.log")).toUtf8();
    char* effArgv[32];
    int effArgc = 0;
    for (int i = 0; i < argc && effArgc < 29; ++i)
    {
        effArgv[effArgc++] = argv[i];
    }
    effArgv[effArgc++] = outArg.data();
    effArgv[effArgc++] = outFile.data();
    int result = QTest::qExec(test, effArgc, effArgv);
    log << "=== " << name << ": " << (result == 0 ? "PASS" : "FAIL") << " (exit=" << result << ") ===\n\n";
    log.flush();
    return result;
}

int main(int argc, char* argv[])
{
    setbuf(stdout, NULL);
    int status = 0;

    // Open log file via QFile — bypasses stdio buffering entirely
    QFile logFile("test_results.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream log(&logFile);

        log << "test_runner.exe started\n";
        log.flush();

        {
            // QApplication (not Core): the param-widget smoke tests need a
            // real GUI platform. We deliberately do NOT force offscreen —
            // converter subprocess tests must keep a normal environment.
            QApplication app(argc, argv);
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

            TestConversionPlanner testPlanner;
            status |= runSuite(&testPlanner, argc, argv, "TestConversionPlanner", log);

            TestConversionCoordinator testCoord;
            status |= runSuite(&testCoord, argc, argv, "TestConversionCoordinator", log);

            TestCliRunner testCli;
            status |= runSuite(&testCli, argc, argv, "TestCliRunner", log);

            TestParamsWidgets testParamsWidgets;
            status |= runSuite(&testParamsWidgets, argc, argv, "TestParamsWidgets", log);

            TestMainWindow testMainWindow;
            status |= runSuite(&testMainWindow, argc, argv, "TestMainWindow", log);
            TestFFmpegConverter testFFmpeg;
            status |= runSuite(&testFFmpeg, argc, argv, "TestFFmpegConverter", log);
            TestFfmpegProgressParser testParser;
            status |= runSuite(&testParser, argc, argv, "TestFfmpegProgressParser", log);
            TestPandocConverter testPandoc;
            status |= runSuite(&testPandoc, argc, argv, "TestPandocConverter", log);
            TestImageMagickConverter testImageMagick;
            status |= runSuite(&testImageMagick, argc, argv, "TestImageMagickConverter", log);
            TestLargeFileHandler testLargeFileHandler;
            status |= runSuite(&testLargeFileHandler, argc, argv, "TestLargeFileHandler", log);
            TestFormatRegistry testFormatRegistry;
            status |= runSuite(&testFormatRegistry, argc, argv, "TestFormatRegistry", log);

            TestPortableMode testPortable;
            status |= runSuite(&testPortable, argc, argv, "TestPortableMode", log);

            // Clean shutdown of singletons in reverse dependency order to prevent
            // hangs during static destruction (TaskManager thread pool)
            TaskManager::instance()->cancelAllTasks();
            appLogger.setFileOutput(false);

            log << "FINAL STATUS: " << status << " (" << (status == 0 ? "ALL PASS" : "SOME FAILURES") << ")\n";
            log.flush();
        }
        logFile.close();
    }

    // Clean return: the g_logger atomic is cleared in ~Logger, so late
    // LOG_* calls from static destructors (TaskManager) are safe no-ops.
    // The old _Exit() hack masked that UB and skipped leak-detection atexit
    // handlers.
    std::fflush(stdout);
    return status;
}
