#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "error_types.h"
#include <QObject>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <functional>

class ConversionTask;

class ErrorHandler : public QObject {
    Q_OBJECT

public:
    static ErrorHandler* instance();
    void handleError(const ErrorInfo& error);
    void handleException(const std::exception& e, const QString& context);
    void handleErrorForTask(const ErrorInfo& error, const QString& taskId);
    QList<ErrorInfo> recentErrors() const;
    QList<ErrorInfo> errorsForTask(const QString& taskId) const;
    ErrorInfo lastError() const;
    void clearErrors();
    void clearErrorsForTask(const QString& taskId);
    bool canRecover(const ErrorInfo& error) const;
    void attemptRecovery(const ErrorInfo& error);
    void setRecoveryHandler(ErrorCode code, std::function<void(const ErrorInfo&)> handler);
    void setMaxStoredErrors(int max);
    int errorCount() const;
    int errorCountForTask(const QString& taskId) const;
    bool hasErrors() const;
    void setShowDialogs(bool show);
    bool showDialogs() const;
    void setAutoRecover(bool autoRecover);
    bool autoRecover() const;

signals:
    void errorOccurred(const ErrorInfo& error);
    void errorForTask(const ErrorInfo& error, const QString& taskId);
    void recoverySuggested(const ErrorInfo& error, const QString& suggestion);
    void recoveryAttempted(const ErrorInfo& error, bool success);
    void errorsCleared();

private slots:
    void onAutoRecoverTimer();

private:
    ErrorHandler();
    ~ErrorHandler();
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    void storeError(const ErrorInfo& error);
    void notifyError(const ErrorInfo& error);
    void trimErrors();
    QList<ErrorInfo> m_errors;
    QMap<ErrorCode, std::function<void(const ErrorInfo&)>> m_recoveryHandlers;
    mutable QMutex m_mutex;
    int m_maxStoredErrors;
    bool m_showDialogs;
    bool m_autoRecover;
    QTimer* m_autoRecoverTimer;
};

#endif // ERROR_HANDLER_H
