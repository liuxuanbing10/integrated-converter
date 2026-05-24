#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "ui/main_window.h"
#include "core/logger.h"
#include "core/config_manager.h"
#include "core/task_manager.h"
#include "converters/ffmpeg_converter.h"
#include "converters/pandoc_converter.h"
#include <memory>
int main(int argc, char* argv[]) {
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
    TaskManager::instance()->registerConverter("FFmpeg", ffmpegConverter);
    TaskManager::instance()->registerConverter("Pandoc", pandocConverter);
    LOG_INFO("Main", "转换器注册完成");
    MainWindow mainWindow;
    mainWindow.show();
    int result = app.exec();
    ConfigManager::instance().saveConfig(configPath);
    LOG_INFO("Main", QString("应用程序退出，返回码: %1").arg(result));
    return result;
}
