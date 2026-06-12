#ifndef TEST_RETRY_MANAGER_H
#define TEST_RETRY_MANAGER_H

#include <QObject>
#include <QString>

class TestRetryManager : public QObject {
    Q_OBJECT
public:
    explicit TestRetryManager(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void testSingletonInstance();
    void testDefaultMaxRetries();
    void testSetMaxRetries();
    void testSetRetryDelay();
    void testSetBackoffMultiplier();
    void testIsRetryable();
    void testIsRetryableNonRecoverable();
    void testScheduleRetryDisabled();
    void testHasPendingRetry();
    void testPendingRetryCountInitial();
    void testRetryCountUnknown();
    void testRetryableErrorCodes();
    void testPendingRetryTasksInitial();
};

#endif // TEST_RETRY_MANAGER_H
