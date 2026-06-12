#include "test_retry_manager.h"
#include "core/retry_manager.h"
#include "core/error_types.h"
#include <QTest>

void TestRetryManager::testSingletonInstance() {
    RetryManager* r1 = RetryManager::instance();
    RetryManager* r2 = RetryManager::instance();
    QVERIFY(r1 != nullptr);
    QVERIFY(r1 == r2);
}

void TestRetryManager::testDefaultMaxRetries() {
    QCOMPARE(RetryManager::instance()->maxRetries(), 3);
}

void TestRetryManager::testSetMaxRetries() {
    RetryManager* mgr = RetryManager::instance();
    mgr->setMaxRetries(5);
    QCOMPARE(mgr->maxRetries(), 5);
    mgr->setMaxRetries(0);
    QCOMPARE(mgr->maxRetries(), 0);
    mgr->setMaxRetries(3);
}

void TestRetryManager::testSetRetryDelay() {
    RetryManager* mgr = RetryManager::instance();
    mgr->setRetryDelay(500, 10000);
    QCOMPARE(mgr->baseRetryDelay(), 500);
    QCOMPARE(mgr->maxRetryDelay(), 10000);
    mgr->setRetryDelay(50, 100);
    QCOMPARE(mgr->baseRetryDelay(), 100);
    mgr->setRetryDelay(1000, 30000);
}

void TestRetryManager::testSetBackoffMultiplier() {
    RetryManager* mgr = RetryManager::instance();
    mgr->setBackoffMultiplier(3.0);
    QCOMPARE(mgr->backoffMultiplier(), 3.0);
    mgr->setBackoffMultiplier(2.0);
}

void TestRetryManager::testIsRetryable() {
    RetryManager* mgr = RetryManager::instance();
    ErrorInfo error;
    error.code = ErrorCode::ConversionFailed;
    error.recoverable = true;
    QVERIFY(mgr->isRetryable(error));
}

void TestRetryManager::testIsRetryableNonRecoverable() {
    RetryManager* mgr = RetryManager::instance();
    ErrorInfo error;
    error.code = ErrorCode::ConversionFailed;
    error.recoverable = false;
    QVERIFY(!mgr->isRetryable(error));
}

void TestRetryManager::testScheduleRetryDisabled() {
    RetryManager* mgr = RetryManager::instance();
    mgr->setEnabled(false);
    ErrorInfo error;
    error.code = ErrorCode::ConversionFailed;
    error.recoverable = true;
    mgr->scheduleRetry("test_task", error);
    QVERIFY(!mgr->hasPendingRetry("test_task"));
    mgr->setEnabled(true);
}

void TestRetryManager::testHasPendingRetry() {
    RetryManager* mgr = RetryManager::instance();
    QVERIFY(!mgr->hasPendingRetry("nonexistent_task"));
}

void TestRetryManager::testPendingRetryCountInitial() {
    QCOMPARE(RetryManager::instance()->pendingRetryCount(), 0);
}

void TestRetryManager::testRetryCountUnknown() {
    QCOMPARE(RetryManager::instance()->retryCount("nonexistent_task"), 0);
}

void TestRetryManager::testRetryableErrorCodes() {
    RetryManager* mgr = RetryManager::instance();
    QList<ErrorCode> codes = mgr->retryableErrorCodes();
    QVERIFY(codes.contains(ErrorCode::ConversionFailed));
    QVERIFY(codes.contains(ErrorCode::TaskTimeout));
    QVERIFY(codes.contains(ErrorCode::ProcessCrashed));
}

void TestRetryManager::testPendingRetryTasksInitial() {
    QVERIFY(RetryManager::instance()->pendingRetryTasks().isEmpty());
}
