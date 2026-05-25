#include <QTest>
#include <QApplication>
#include "core/test_logger.h"
#include "core/test_config_manager.h"
#include "core/test_task_manager.h"
#include "core/test_error_handler.h"
#include "converters/test_ffmpeg_converter.h"
#include "converters/test_pandoc_converter.h"
#include "converters/test_imagemagick_converter.h"
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
    {
        TestImageMagickConverter testImageMagick;
        status |= QTest::qExec(&testImageMagick, argc, argv);
    }
    return status;
}
