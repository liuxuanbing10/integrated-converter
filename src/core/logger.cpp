#include "logger.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QFileInfoList>
#include <algorithm>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_level(Level::Info)
    , m_consoleOutput(true)
    , m_fileOutput(false)
    , m_maxFileSize(10 * 1024 * 1024)
    , m_maxBackupFiles(5)
    , m_useModuleFilter(false)
{
}

Logger::~Logger() {
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }
}

void Logger::setLogFile(const QString& filePath) {
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_logFilePath = filePath;
    m_logFile.setFileName(filePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_stream.setDevice(&m_logFile);
        m_fileOutput = true;
    } else {
        m_fileOutput = false;
        qWarning() << "Failed to open log file:" << filePath;
    }
}

void Logger::setLevel(Level level) {
    QMutexLocker locker(&m_mutex);
    m_level = level;
}

Logger::Level Logger::level() const {
    QMutexLocker locker(&m_mutex);
    return m_level;
}

void Logger::closeLogFile() {
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }
    m_fileOutput = false;
}

void Logger::setConsoleOutput(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = enabled;
}

void Logger::setFileOutput(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_fileOutput = enabled;
}

void Logger::setMaxFileSize(qint64 maxSize) {
    QMutexLocker locker(&m_mutex);
    m_maxFileSize = maxSize;
}

qint64 Logger::maxFileSize() const {
    QMutexLocker locker(&m_mutex);
    return m_maxFileSize;
}

void Logger::setMaxBackupFiles(int count) {
    QMutexLocker locker(&m_mutex);
    m_maxBackupFiles = qMax(0, count);
}

int Logger::maxBackupFiles() const {
    QMutexLocker locker(&m_mutex);
    return m_maxBackupFiles;
}

void Logger::enableModule(const QString& module) {
    QMutexLocker locker(&m_mutex);
    m_enabledModules.insert(module);
    m_useModuleFilter = true;
}

void Logger::disableModule(const QString& module) {
    QMutexLocker locker(&m_mutex);
    m_enabledModules.remove(module);
}

void Logger::setModuleFilter(const QSet<QString>& modules) {
    QMutexLocker locker(&m_mutex);
    m_enabledModules = modules;
    m_useModuleFilter = !modules.isEmpty();
}

void Logger::clearModuleFilter() {
    QMutexLocker locker(&m_mutex);
    m_enabledModules.clear();
    m_useModuleFilter = false;
}

bool Logger::isModuleEnabled(const QString& module) const {
    QMutexLocker locker(&m_mutex);
    if (!m_useModuleFilter) {
        return true;
    }
    return m_enabledModules.contains(module);
}

void Logger::log(Level level, const QString& module, const QString& message) {
    if (level < m_level) {
        return;
    }
    if (!isModuleEnabled(module)) {
        return;
    }
    QMutexLocker locker(&m_mutex);
    QString formatted = formatMessage(level, module, message);
    if (m_consoleOutput) {
        switch (level) {
            case Level::Debug:    qDebug().noquote() << formatted; break;
            case Level::Info:     qInfo().noquote() << formatted; break;
            case Level::Warning:  qWarning().noquote() << formatted; break;
            case Level::Error:    qCritical().noquote() << formatted; break;
        }
    }
    if (m_fileOutput && m_logFile.isOpen()) {
        writeToFile(formatted);
    }
}

void Logger::debug(const QString& module, const QString& message) {
    log(Level::Debug, module, message);
}

void Logger::info(const QString& module, const QString& message) {
    log(Level::Info, module, message);
}

void Logger::warning(const QString& module, const QString& message) {
    log(Level::Warning, module, message);
}

void Logger::error(const QString& module, const QString& message) {
    log(Level::Error, module, message);
}

QString Logger::levelToString(Level level) const {
    switch (level) {
        case Level::Debug:   return QStringLiteral("DEBUG");
        case Level::Info:    return QStringLiteral("INFO");
        case Level::Warning: return QStringLiteral("WARNING");
        case Level::Error:   return QStringLiteral("ERROR");
        default:             return QStringLiteral("UNKNOWN");
    }
}

QString Logger::formatMessage(Level level, const QString& module, const QString& message) const {
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyy-MM-dd hh:mm:ss.zzz");
    return QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelToString(level), -7)
        .arg(module)
        .arg(message);
}

void Logger::rotateLog() {
    if (!m_logFile.isOpen()) {
        return;
    }
    m_stream.flush();
    m_logFile.close();
    QString backupName = generateBackupFileName();
    if (QFile::rename(m_logFilePath, backupName)) {
        cleanupOldBackups();
    }
    m_logFile.setFileName(m_logFilePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_stream.setDevice(&m_logFile);
    } else {
        m_fileOutput = false;
        qWarning() << "Failed to reopen log file after rotation:" << m_logFilePath;
    }
}

void Logger::writeToFile(const QString& formattedMessage) {
    if (m_logFile.size() >= m_maxFileSize) {
        rotateLog();
    }
    if (m_logFile.isOpen()) {
        QByteArray line = (formattedMessage + '\n').toUtf8();
        m_logFile.write(line);
        m_logFile.flush();
    }
}

QString Logger::generateBackupFileName() const {
    QFileInfo fileInfo(m_logFilePath);
    QString baseDir = fileInfo.absolutePath();
    QString baseName = fileInfo.completeBaseName();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("%1/backup_%2_%3.log")
        .arg(baseDir)
        .arg(baseName)
        .arg(timestamp);
}

void Logger::cleanupOldBackups() {
    if (m_maxBackupFiles <= 0) {
        return;
    }
    QFileInfo fileInfo(m_logFilePath);
    QString baseDir = fileInfo.absolutePath();
    QString baseName = fileInfo.completeBaseName();
    QString pattern = QString("backup_%1_*.log").arg(baseName);
    QDir dir(baseDir);
    QStringList filters;
    filters << pattern;
    QFileInfoList backupFiles = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    while (backupFiles.size() > m_maxBackupFiles) {
        QFile::remove(backupFiles.first().absoluteFilePath());
        backupFiles.removeFirst();
    }
}
