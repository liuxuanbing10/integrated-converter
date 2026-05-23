#ifndef RETRY_MANAGER_H
#define RETRY_MANAGER_H

#include "error_types.h"
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QQueue>
#include <QMutex>

class ConversionTask;

struct RetryInfo {
    QString taskId;
    ErrorInfo lastError;
    int retryCount;
    int maxRetries;
    int nextDelayMs;
    int baseDelayMs;
    int maxDelayMs;
    double backoffMultiplier;
    RetryInfo() : retryCount(0), maxRetries(3), nextDelayMs(1000),
                  baseDelayMs(1000), maxDelayMs(30000), backoffMultiplier(2.0) {}
};

class RetryManager : public QObject {
    Q_OBJECT

public:
    static RetryManager* instance();
    void scheduleRetry(const QString& taskId, const ErrorInfo& error, int delayMs = 0);
    void scheduleRetryImmediate(const QString& taskId, const ErrorInfo& error);
    void cancelRetry(const QString& taskId);
    void cancelAllRetries();
    void setMaxRetries(int max);
    int maxRetries() const { return m_maxRetries; }
    void setRetryDelay(int baseMs, int maxMs);
    int baseRetryDelay() const { return m_baseDelayMs; }
    int maxRetryDelay() const { return m_maxDelayMs; }
    void setBackoffMultiplier(double multiplier);
    double backoffMultiplier() const { return m_backoffMultiplier; }
    void setRetryableErrorCodes(const QList<ErrorCode>& codes);
    QList<ErrorCode> retryableErrorCodes() const;
    bool isRetryable(const ErrorInfo& error) const;
    int retryCount(const QString& taskId) const;
    bool hasPendingRetry(const QString& taskId) const;
    QList<QString> pendingRetryTasks() const;
    int pendingRetryCount() const;
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

signals:
    void retryScheduled(const QString& taskId, int delayMs);
    void retryTriggered(const QString& taskId, int retryCount);
    void retryExhausted(const QString& taskId, const ErrorInfo& lastError);
    void retryCancelled(const QString& taskId);

private slots:
    void onRetryTimer();

private:
    RetryManager();
    ~RetryManager();
    RetryManager(const RetryManager&) = delete;
    RetryManager& operator=(const RetryManager&) = delete;
    int calculateNextDelay(const RetryInfo& info) const;
    QMap<QString, RetryInfo> m_retryInfos;
    QMap<QString, QTimer*> m_retryTimers;
    QList<ErrorCode> m_retryableCodes;
    mutable QMutex m_mutex;
    int m_maxRetries;
    int m_baseDelayMs;
    int m_maxDelayMs;
    double m_backoffMultiplier;
    bool m_enabled;
};

#endif // RETRY_MANAGER_H
