#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QStandardPaths>
#include <QMutex>
#include <QStringList>
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
#include "cli/cli_runner.h"
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
    if (hConsole != INVALID_HANDLE_VALUE && hConsole != nullptr) {
        // WriteConsoleW requires a real console handle. If stderr is redirected
        // to a file/pipe, fall back to a UTF-8 narrow write.
        if (GetFileType(hConsole) == FILE_TYPE_CHAR) {
            WriteConsoleW(hConsole, fullMsg.utf16(), fullMsg.size(), &written, nullptr);
        } else {
            const QByteArray utf8 = fullMsg.toUtf8();
            fwrite(utf8.constData(), 1, utf8.size(), stderr);
            fflush(stderr);
        }
    }
    if (type == QtFatalMsg) {
        // Let the default handler deal with abort
        abort();
    }
}
#endif

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    // Install custom handler that uses WriteConsoleW (UTF-16 native) to avoid
    // garbled Chinese output caused by Qt's default GBK-to-console encoding.
    // Note: we deliberately do NOT call SetConsoleOutputCP(CP_UTF8) here —
    // WriteConsoleW bypasses the console code page entirely, so changing CP
    // would only affect narrow writes (printf/fprintf) which we don't use.
    qInstallMessageHandler(winConsoleMessageHandler);
#endif

    // ----------------------------------------------------------------
    // CLI mode: detect --cli / --help / --list-formats BEFORE creating
    // QApplication and BEFORE bringing up the heavy singletons
    // (MemoryMonitor, TaskManager, ConfigManager). Those spin up QTimer-
    // backed threads that complicate clean exit on `return`.
    // ----------------------------------------------------------------
    bool isCli = false;
    for (int i = 1; i < argc; ++i) {
        QString a = QString::fromLocal8Bit(argv[i]);
        if (a == "--cli" || a == "--help" || a == "-h" || a == "--list-formats") {
            isCli = true;
            break;
        }
    }

    if (isCli) {
        // QCoreApplication is sufficient — converters don't need GUI.
        QCoreApplication coreApp(argc, argv);
        coreApp.setApplicationName("IntegratedConverter");
        coreApp.setApplicationVersion("1.0.0");
        coreApp.setOrganizationName("ConverterTools");

        auto ffmpegConverter = std::make_shared<FFmpegConverter>();
        auto pandocConverter = std::make_shared<PandocConverter>();
        auto imagemagickConverter = std::make_shared<ImageMagickConverter>();
        QHash<QString, void*> byName;
        byName.insert("FFmpeg",      ffmpegConverter.get());
        byName.insert("Pandoc",      pandocConverter.get());
        byName.insert("ImageMagick", imagemagickConverter.get());

        QStringList cliArgs = coreApp.arguments().mid(1);
        // Strip the "--cli" sentinel that triggered this branch — the parser
        // treats it as positional and would miscount it as an input.
        cliArgs.removeAll("--cli");
        QString err;
        CliRunner::Options opts = CliRunner::parseArgs(cliArgs, &err);
        if (!err.isEmpty()) {
            QTextStream(stderr) << err << "\n\n";
            CliRunner::printHelp();
            return 2;
        }
        if (opts.showHelp) {
            CliRunner::printHelp();
            return 0;
        }
        if (opts.listFormats) {
            CliRunner::printFormats();
            return 0;
        }
        QString runErr;
        int rc = CliRunner::run(opts, byName, &runErr);
        if (!runErr.isEmpty()) {
            QTextStream(stderr) << runErr << "\n";
        }
        return rc;
    }

    // ----------------------------------------------------------------
    // GUI mode: full initialization.
    // ----------------------------------------------------------------
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

    // Requirement 5: ensure console closes with the GUI application
#ifdef Q_OS_WIN
    // On MinGW builds, WIN32_EXECUTABLE is OFF, so a console is allocated.
    // FreeConsole detaches the process from its console.
    // If the console was created specifically for this process (not inherited
    // from an existing cmd.exe), the console window will close automatically.
    FreeConsole();
#endif

    ConfigManager::instance().saveConfig(configPath);
    LOG_INFO("Main", QString("应用程序退出，返回码: %1").arg(result));
    return result;
}
