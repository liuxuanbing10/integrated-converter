#include "retry_manager.h"
#include "logger.h"
#include <QThread>

RetryManager* RetryManager::instance() {
    static RetryManager s_instance;
    return &s_instance;
}

RetryManager::RetryManager()
    : m_maxRetries(3)
    , m_baseDelayMs(1000)
    , m_maxDelayMs(30000)
    , m_backoffMultiplier(2.0)
    , m_enabled(true)
{
    m_retryableCodes = {
        ErrorCode::ConversionFailed,
        ErrorCode::TaskTimeout,
        ErrorCode::ProcessCrashed,
        ErrorCode::FileNotFound,
        ErrorCode::PermissionDenied
    };
    LOG_INFO("RetryManager", "重试管理器初始化完成");
}

RetryManager::~RetryManager() {
    cancelAllRetries();
}

void RetryManager::scheduleRetry(const QString& taskId, const ErrorInfo& error, int delayMs) {
    if (!m_enabled) {
        LOG_DEBUG("RetryManager", QString("重试已禁用，跳过任务: %1").arg(taskId));
        return;
    }
    if (!isRetryable(error)) {
        LOG_DEBUG("RetryManager", QString("错误不可重试: %1")
                 .arg(ErrorTypes::errorCodeToString(error.code)));
        emit retryExhausted(taskId, error);
        return;
    }
    QMutexLocker locker(&m_mutex);
    RetryInfo info;
    if (m_retryInfos.contains(taskId)) {
        info = m_retryInfos[taskId];
        info.retryCount++;
    } else {
        info.taskId = taskId;
        info.retryCount = 1;
        info.maxRetries = m_maxRetries;
        info.baseDelayMs = m_baseDelayMs;
        info.maxDelayMs = m_maxDelayMs;
        info.backoffMultiplier = m_backoffMultiplier;
    }
    info.lastError = error;
    if (info.retryCount > info.maxRetries) {
        LOG_WARNING("RetryManager", QString("任务重试次数已达上限: %1 (次数: %2)")
                   .arg(taskId).arg(info.retryCount - 1));
        m_retryInfos.remove(taskId);
        if (m_retryTimers.contains(taskId)) {
            m_retryTimers[taskId]->stop();
            m_retryTimers[taskId]->deleteLater();
            m_retryTimers.remove(taskId);
        }
        locker.unlock();
        emit retryExhausted(taskId, error);
        return;
    }
    if (delayMs <= 0) {
        delayMs = calculateNextDelay(info);
    }
    info.nextDelayMs = delayMs;
    m_retryInfos[taskId] = info;
    if (m_retryTimers.contains(taskId)) {
        m_retryTimers[taskId]->stop();
    } else {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, &RetryManager::onRetryTimer);
        m_retryTimers[taskId] = timer;
    }
    m_retryTimers[taskId]->start(delayMs);
    LOG_INFO("RetryManager", QString("调度重试: %1, 延迟: %2ms, 次数: %3/%4")
            .arg(taskId).arg(delayMs).arg(info.retryCount).arg(info.maxRetries));
    locker.unlock();
    emit retryScheduled(taskId, delayMs);
}

void RetryManager::scheduleRetryImmediate(const QString& taskId, const ErrorInfo& error) {
    scheduleRetry(taskId, error, 0);
}

void RetryManager::cancelRetry(const QString& taskId) {
    QMutexLocker locker(&m_mutex);
    if (m_retryTimers.contains(taskId)) {
        m_retryTimers[taskId]->stop();
        m_retryTimers[taskId]->deleteLater();
        m_retryTimers.remove(taskId);
    }
    m_retryInfos.remove(taskId);
    locker.unlock();
    LOG_INFO("RetryManager", QString("取消重试: %1").arg(taskId));
    emit retryCancelled(taskId);
}

void RetryManager::cancelAllRetries() {
    QMutexLocker locker(&m_mutex);
    for (auto it = m_retryTimers.begin(); it != m_retryTimers.end(); ++it) {
        it.value()->stop();
        it.value()->deleteLater();
    }
    m_retryTimers.clear();
    m_retryInfos.clear();
    LOG_INFO("RetryManager", "已取消所有重试");
}

void RetryManager::setMaxRetries(int max) {
    m_maxRetries = qMax(0, max);
    LOG_INFO("RetryManager", QString("设置最大重试次数: %1").arg(m_maxRetries));
}

void RetryManager::setRetryDelay(int baseMs, int maxMs) {
    m_baseDelayMs = qMax(100, baseMs);
    m_maxDelayMs = qMax(m_baseDelayMs, maxMs);
    LOG_INFO("RetryManager", QString("设置重试延迟: 基础=%1ms, 最大=%2ms")
            .arg(m_baseDelayMs).arg(m_maxDelayMs));
}

void RetryManager::setBackoffMultiplier(double multiplier) {
    m_backoffMultiplier = qBound(1.0, multiplier, 10.0);
}

void RetryManager::setRetryableErrorCodes(const QList<ErrorCode>& codes) {
    m_retryableCodes = codes;
}

QList<ErrorCode> RetryManager::retryableErrorCodes() const {
    return m_retryableCodes;
}

bool RetryManager::isRetryable(const ErrorInfo& error) const {
    return m_retryableCodes.contains(error.code) && error.recoverable;
}

int RetryManager::retryCount(const QString& taskId) const {
    QMutexLocker locker(&m_mutex);
    if (m_retryInfos.contains(taskId)) {
        return m_retryInfos[taskId].retryCount;
    }
    return 0;
}

bool RetryManager::hasPendingRetry(const QString& taskId) const {
    QMutexLocker locker(&m_mutex);
    return m_retryTimers.contains(taskId) && m_retryTimers[taskId]->isActive();
}

QList<QString> RetryManager::pendingRetryTasks() const {
    QMutexLocker locker(&m_mutex);
    return m_retryTimers.keys();
}

int RetryManager::pendingRetryCount() const {
    QMutexLocker locker(&m_mutex);
    return m_retryTimers.size();
}

void RetryManager::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled) {
        cancelAllRetries();
    }
}

void RetryManager::onRetryTimer() {
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;
    QString taskId;
    {
        QMutexLocker locker(&m_mutex);
        for (auto it = m_retryTimers.begin(); it != m_retryTimers.end(); ++it) {
            if (it.value() == timer) {
                taskId = it.key();
                break;
            }
        }
    }
    if (taskId.isEmpty()) return;
    int retryCountVal = 0;
    {
        QMutexLocker locker(&m_mutex);
        if (m_retryInfos.contains(taskId)) {
            retryCountVal = m_retryInfos[taskId].retryCount;
        }
    }
    LOG_INFO("RetryManager", QString("触发重试: %1, 第 %2 次").arg(taskId).arg(retryCountVal));
    emit retryTriggered(taskId, retryCountVal);
}

int RetryManager::calculateNextDelay(const RetryInfo& info) const {
    int delay = static_cast<int>(info.baseDelayMs *
                                 qPow(info.backoffMultiplier, info.retryCount - 1));
    return qMin(delay, info.maxDelayMs);
}
