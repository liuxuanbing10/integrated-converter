#include <QTest>
#include <QApplication>
#include "core/test_logger.cpp"
#include "core/test_config_manager.cpp"
#include "core/test_task_manager.cpp"
#include "core/test_error_handler.cpp"
#include "converters/test_ffmpeg_converter.cpp"
#include "converters/test_pandoc_converter.cpp"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    int status = 0;
    {
        TestLogger testLogger;
        status |= QTest::qExec(&testLogger, argc, argv);
    }
    {
        TestConfigManager testConfig;
        status |= QTest::qExec(&testConfig, argc, argv);
    }
    {
        TestTaskManager testTaskManager;
        status |= QTest::qExec(&testTaskManager, argc, argv);
    }
    {
        TestErrorHandler testErrorHandler;
        status |= QTest::qExec(&testErrorHandler, argc, argv);
    }
    {
        TestFFmpegConverter testFFmpeg;
        status |= QTest::qExec(&testFFmpeg, argc, argv);
    }
    {
        TestPandocConverter testPandoc;
        status |= QTest::qExec(&testPandoc, argc, argv);
    }
    return status;
}
