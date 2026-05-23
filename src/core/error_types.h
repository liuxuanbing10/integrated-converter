#ifndef ERROR_TYPES_H
#define ERROR_TYPES_H

#include <QString>
#include <QDateTime>
#include <QVariant>

enum class ErrorCode {
    Unknown = 0,
    InvalidParameter = 1,
    FileNotFound = 2,
    PermissionDenied = 3,
    DiskSpaceInsufficient = 4,
    ConverterNotFound = 100,
    ConverterNotAvailable = 101,
    UnsupportedFormat = 102,
    ConversionFailed = 103,
    TaskCancelled = 200,
    TaskTimeout = 201,
    TaskDependencyFailed = 202,
    ProcessCrashed = 300,
    OutOfMemory = 301,
    ProcessFailedToStart = 302
};

struct ErrorInfo {
    ErrorCode code = ErrorCode::Unknown;
    QString message;
    QString details;
    QString suggestion;
    QString context;
    QString taskId;
    QString converterName;
    QString inputFile;
    QString outputFile;
    QDateTime timestamp;
    int retryCount = 0;
    bool recoverable = false;
    ErrorInfo() : timestamp(QDateTime::currentDateTime()) {}
    ErrorInfo(ErrorCode c, const QString& msg)
        : code(c), message(msg), timestamp(QDateTime::currentDateTime()) {}
    bool isValid() const { return code != ErrorCode::Unknown || !message.isEmpty(); }
    QString fullMessage() const {
        QString result = message;
        if (!details.isEmpty()) {
            result += "\n" + details;
        }
        return result;
    }
};

namespace ErrorTypes {
    QString errorCodeToString(ErrorCode code);
    ErrorCode stringToErrorCode(const QString& str);
    QString defaultSuggestion(ErrorCode code);
    bool isRecoverable(ErrorCode code);
    ErrorInfo createError(ErrorCode code, const QString& message,
                         const QString& context = QString(),
                         const QString& taskId = QString());
    ErrorInfo createFileNotFoundError(const QString& filePath,
                                     const QString& context = QString());
    ErrorInfo createPermissionDeniedError(const QString& filePath,
                                         const QString& context = QString());
    ErrorInfo createConverterNotAvailableError(const QString& converterName,
                                              const QString& context = QString());
    ErrorInfo createUnsupportedFormatError(const QString& inputFormat,
                                          const QString& outputFormat,
                                          const QString& context = QString());
    ErrorInfo createConversionFailedError(const QString& details,
                                         const QString& converterName = QString(),
                                         const QString& context = QString());
    ErrorInfo createTaskCancelledError(const QString& taskId,
                                      const QString& context = QString());
    ErrorInfo createProcessError(ErrorCode code, const QString& processName,
                                const QString& details = QString(),
                                const QString& context = QString());
}

#endif // ERROR_TYPES_H
