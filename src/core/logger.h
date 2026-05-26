#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QSet>
#include <QFileInfo>

class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    static Logger& instance();

    void setLogFile(const QString& filePath);
    void setLevel(Level level);
    Level level() const;

    void setConsoleOutput(bool enabled);
    void setFileOutput(bool enabled);
    void closeLogFile();

    void setMaxFileSize(qint64 maxSize);
    qint64 maxFileSize() const;

    void setMaxBackupFiles(int count);
    int maxBackupFiles() const;

    void enableModule(const QString& module);
    void disableModule(const QString& module);
    void setModuleFilter(const QSet<QString>& modules);
    void clearModuleFilter();
    bool isModuleEnabled(const QString& module) const;

    void log(Level level, const QString& module, const QString& message);
    void debug(const QString& module, const QString& message);
    void info(const QString& module, const QString& message);
    void warning(const QString& module, const QString& message);
    void error(const QString& module, const QString& message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

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

#define LOG_DEBUG(module, message)   Logger::instance().debug(module, message)
#define LOG_INFO(module, message)    Logger::instance().info(module, message)
#define LOG_WARNING(module, message) Logger::instance().warning(module, message)
#define LOG_ERROR(module, message)   Logger::instance().error(module, message)

#endif // LOGGER_H
