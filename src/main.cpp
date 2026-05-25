#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "ui/main_window.h"
#include "core/logger.h"
#include "core/config_manager.h"
#include "core/task_manager.h"
#include "converters/ffmpeg_converter.h"
#include "converters/pandoc_converter.h"
#include "converters/imagemagick_converter.h"
#include <memory>
int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    // Set console output to UTF-8 to prevent garbled Chinese text
    SetConsoleOutputCP(CP_UTF8);
#endif
    QApplication app(argc, argv);
    app.setApplicationName("IntegratedConverter");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ConverterTools");
    QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QString logPath = configDir + "/converter.log";
    Logger::instance().setLogFile(logPath);
    Logger::instance().setLevel(Logger::Level::Info);
    LOG_INFO("Main", "应用程序启动");
    QString configPath = configDir + "/config.json";
    if (QFile::exists(configPath)) {
        ConfigManager::instance().loadConfig(configPath);
    }
    int logLevel = ConfigManager::instance().logLevel();
    Logger::instance().setLevel(static_cast<Logger::Level>(logLevel));
    auto ffmpegConverter = std::make_shared<FFmpegConverter>();
    auto pandocConverter = std::make_shared<PandocConverter>();
    auto imagemagickConverter = std::make_shared<ImageMagickConverter>();
    TaskManager::instance()->registerConverter("FFmpeg", ffmpegConverter);
    TaskManager::instance()->registerConverter("Pandoc", pandocConverter);
    TaskManager::instance()->registerConverter("ImageMagick", imagemagickConverter);
    LOG_INFO("Main", "转换器注册完成");
    MainWindow mainWindow;
    mainWindow.show();
    int result = app.exec();
    ConfigManager::instance().saveConfig(configPath);
    LOG_INFO("Main", QString("应用程序退出，返回码: %1").arg(result));
    return result;
}
