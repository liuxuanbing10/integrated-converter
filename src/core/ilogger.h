#ifndef ILOGGER_H
#define ILOGGER_H

#include <QString>
#include <QSet>

class ILogger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    virtual ~ILogger() = default;

    virtual void setLogFile(const QString& filePath) = 0;
    virtual void setLevel(Level level) = 0;
    virtual Level level() const = 0;

    virtual void setConsoleOutput(bool enabled) = 0;
    virtual void setFileOutput(bool enabled) = 0;
    virtual void closeLogFile() = 0;

    virtual void setMaxFileSize(qint64 maxSize) = 0;
    virtual qint64 maxFileSize() const = 0;

    virtual void setMaxBackupFiles(int count) = 0;
    virtual int maxBackupFiles() const = 0;

    virtual void enableModule(const QString& module) = 0;
    virtual void disableModule(const QString& module) = 0;
    virtual void setModuleFilter(const QSet<QString>& modules) = 0;
    virtual void clearModuleFilter() = 0;
    virtual bool isModuleEnabled(const QString& module) const = 0;

    virtual void log(Level level, const QString& module, const QString& message) = 0;
    virtual void debug(const QString& module, const QString& message) = 0;
    virtual void info(const QString& module, const QString& message) = 0;
    virtual void warning(const QString& module, const QString& message) = 0;
    virtual void error(const QString& module, const QString& message) = 0;
};

/// Global logger pointer used by LOG_* convenience macros.
/// Set at application startup (main.cpp) before any logging occurs.
extern ILogger* g_logger;

#endif // ILOGGER_H
