#include "error_handler.h"
#include "logger.h"
#include <QApplication>
#include <QMessageBox>
#include <algorithm>

ErrorHandler* ErrorHandler::instance() {
    static ErrorHandler* s_instance = nullptr;
    if (!s_instance) {
        s_instance = new ErrorHandler();
    }
    return s_instance;
}

ErrorHandler::ErrorHandler()
    : m_maxStoredErrors(100)
    , m_showDialogs(true)
    , m_autoRecover(false)
    , m_autoRecoverTimer(new QTimer(this))
{
    m_autoRecoverTimer->setSingleShot(true);
    connect(m_autoRecoverTimer, &QTimer::timeout, this, &ErrorHandler::onAutoRecoverTimer);
    LOG_INFO("ErrorHandler", "错误处理器初始化完成");
}

ErrorHandler::~ErrorHandler() {
}

void ErrorHandler::handleError(const ErrorInfo& error) {
    if (!error.isValid()) {
        return;
    }
    LOG_ERROR("ErrorHandler", QString("错误: %1 - %2")
              .arg(ErrorTypes::errorCodeToString(error.code), error.message));
    storeError(error);
    notifyError(error);
}

void ErrorHandler::handleException(const std::exception& e, const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::Unknown;
    error.message = QString::fromStdString(e.what());
    error.context = context;
    error.suggestion = tr("程序发生异常，请检查输入参数或联系开发者");
    handleError(error);
}

void ErrorHandler::handleErrorForTask(const ErrorInfo& error, const QString& taskId) {
    ErrorInfo taskError = error;
    taskError.taskId = taskId;
    handleError(taskError);
    emit errorForTask(taskError, taskId);
}

QList<ErrorInfo> ErrorHandler::recentErrors() const {
    QMutexLocker locker(&m_mutex);
    return m_errors;
}

QList<ErrorInfo> ErrorHandler::errorsForTask(const QString& taskId) const {
    QMutexLocker locker(&m_mutex);
    QList<ErrorInfo> result;
    for (const ErrorInfo& error : m_errors) {
        if (error.taskId == taskId) {
            result.append(error);
        }
    }
    return result;
}

ErrorInfo ErrorHandler::lastError() const {
    QMutexLocker locker(&m_mutex);
    if (m_errors.isEmpty()) {
        return ErrorInfo();
    }
    return m_errors.last();
}

void ErrorHandler::clearErrors() {
    QMutexLocker locker(&m_mutex);
    m_errors.clear();
    emit errorsCleared();
    LOG_INFO("ErrorHandler", "已清除所有错误记录");
}

void ErrorHandler::clearErrorsForTask(const QString& taskId) {
    QMutexLocker locker(&m_mutex);
    m_errors.erase(std::remove_if(m_errors.begin(), m_errors.end(),
                   [&taskId](const ErrorInfo& e) { return e.taskId == taskId; }),
                   m_errors.end());
}

bool ErrorHandler::canRecover(const ErrorInfo& error) const {
    return error.recoverable && m_recoveryHandlers.contains(error.code);
}

void ErrorHandler::attemptRecovery(const ErrorInfo& error) {
    if (!canRecover(error)) {
        LOG_WARNING("ErrorHandler", QString("无法恢复错误: %1")
                   .arg(ErrorTypes::errorCodeToString(error.code)));
        emit recoveryAttempted(error, false);
        return;
    }
    LOG_INFO("ErrorHandler", QString("尝试恢复错误: %1")
            .arg(ErrorTypes::errorCodeToString(error.code)));
    try {
        m_recoveryHandlers[error.code](error);
        emit recoveryAttempted(error, true);
        LOG_INFO("ErrorHandler", "错误恢复成功");
    } catch (const std::exception& e) {
        LOG_ERROR("ErrorHandler", QString("恢复失败: %1").arg(e.what()));
        emit recoveryAttempted(error, false);
    }
}

void ErrorHandler::setRecoveryHandler(ErrorCode code,
                                     std::function<void(const ErrorInfo&)> handler) {
    m_recoveryHandlers[code] = handler;
}

void ErrorHandler::setMaxStoredErrors(int max) {
    m_maxStoredErrors = qMax(1, max);
    trimErrors();
}

int ErrorHandler::errorCount() const {
    QMutexLocker locker(&m_mutex);
    return m_errors.size();
}

int ErrorHandler::errorCountForTask(const QString& taskId) const {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (const ErrorInfo& error : m_errors) {
        if (error.taskId == taskId) {
            ++count;
        }
    }
    return count;
}

bool ErrorHandler::hasErrors() const {
    QMutexLocker locker(&m_mutex);
    return !m_errors.isEmpty();
}

void ErrorHandler::setShowDialogs(bool show) {
    m_showDialogs = show;
}

bool ErrorHandler::showDialogs() const {
    return m_showDialogs;
}

void ErrorHandler::setAutoRecover(bool autoRecover) {
    m_autoRecover = autoRecover;
}

bool ErrorHandler::autoRecover() const {
    return m_autoRecover;
}

void ErrorHandler::onAutoRecoverTimer() {
    if (m_autoRecover && hasErrors()) {
        ErrorInfo error = lastError();
        if (canRecover(error)) {
            attemptRecovery(error);
        }
    }
}

void ErrorHandler::storeError(const ErrorInfo& error) {
    QMutexLocker locker(&m_mutex);
    m_errors.append(error);
    trimErrors();
}

void ErrorHandler::notifyError(const ErrorInfo& error) {
    emit errorOccurred(error);
    if (!error.suggestion.isEmpty()) {
        emit recoverySuggested(error, error.suggestion);
    }
    if (m_autoRecover && canRecover(error)) {
        m_autoRecoverTimer->start(1000);
    }
}

void ErrorHandler::trimErrors() {
    while (m_errors.size() > m_maxStoredErrors) {
        m_errors.removeFirst();
    }
}
