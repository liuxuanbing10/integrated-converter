#ifndef LOGGER_H
#define LOGGER_H

#include "ilogger.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QSet>
#include <QFileInfo>

class Logger : public ILogger {
public:
    Logger();
    ~Logger() override;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void setLogFile(const QString& filePath) override;
    void setLevel(Level level) override;
    Level level() const override;

    void setConsoleOutput(bool enabled) override;
    void setFileOutput(bool enabled) override;
    void closeLogFile() override;

    void setMaxFileSize(qint64 maxSize) override;
    qint64 maxFileSize() const override;

    void setMaxBackupFiles(int count) override;
    int maxBackupFiles() const override;

    void enableModule(const QString& module) override;
    void disableModule(const QString& module) override;
    void setModuleFilter(const QSet<QString>& modules) override;
    void clearModuleFilter() override;
    bool isModuleEnabled(const QString& module) const override;

    void log(Level level, const QString& module, const QString& message) override;
    void debug(const QString& module, const QString& message) override;
    void info(const QString& module, const QString& message) override;
    void warning(const QString& module, const QString& message) override;
    void error(const QString& module, const QString& message) override;

private:
    QString levelToString(Level level) const;
    QString formatMessage(Level level, const QString& module, const QString& message) const;
    void rotateLog();
    void writeToFile(const QString& formattedMessage);
    void cleanupOldBackups();
    QString generateBackupFileName() const;

    friend class TestLogger;

    QFile m_logFile;
    QTextStream m_stream;
    mutable QMutex m_mutex;
    Level m_level;
    bool m_consoleOutput;
    bool m_fileOutput;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;
    QString m_logFilePath;
    QSet<QString> m_enabledModules;
    bool m_useModuleFilter;
};

// Convenience macros — check g_logger before dereferencing so they
// safely degrade to a no-op when no logger has been installed.
#define LOG_DEBUG(module, message)   do { if (::g_logger) ::g_logger->debug(module, message); } while(0)
#define LOG_INFO(module, message)    do { if (::g_logger) ::g_logger->info(module, message); } while(0)
#define LOG_WARNING(module, message) do { if (::g_logger) ::g_logger->warning(module, message); } while(0)
#define LOG_ERROR(module, message)   do { if (::g_logger) ::g_logger->error(module, message); } while(0)

#endif // LOGGER_H
