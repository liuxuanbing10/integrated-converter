#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QMutex>
#include <iostream>
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

#ifdef Q_OS_WIN
// Custom Qt message handler that writes UTF-16 directly to the Windows console,
// bypassing code page conversion issues that cause garbled Chinese text.
static QMutex g_consoleMutex;
static void winConsoleMessageHandler(QtMsgType type, const QMessageLogContext& ctx,
                                       const QString& msg) {
    QMutexLocker locker(&g_consoleMutex);
    QString prefix;
    switch (type) {
        case QtDebugMsg:    prefix = "[DEBUG] "; break;
        case QtInfoMsg:     prefix = "[INFO] ";  break;
        case QtWarningMsg:  prefix = "[WARN] ";  break;
        case QtCriticalMsg: prefix = "[ERROR] "; break;
        case QtFatalMsg:    prefix = "[FATAL] "; break;
    }
    QString fullMsg = prefix + msg + "\n";
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    DWORD written;
    WriteConsoleW(hConsole, fullMsg.utf16(), fullMsg.size(), &written, nullptr);
    if (type == QtFatalMsg) {
        // Let the default handler deal with abort
        fflush(stderr);
        abort();
    }
}
#endif

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    // Install custom handler that uses WriteConsoleW (UTF-16 native) to avoid
    // garbled Chinese output caused by Qt's default GBK-to-console encoding.
    qInstallMessageHandler(winConsoleMessageHandler);
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
