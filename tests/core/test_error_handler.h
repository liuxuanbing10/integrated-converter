#ifndef TEST_ERROR_HANDLER_H
#define TEST_ERROR_HANDLER_H

#include <QObject>

class TestErrorHandler : public QObject {
    Q_OBJECT
public:
    explicit TestErrorHandler(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testHandleError();
    void testHandleMultipleErrors();
    void testHandleException();
    void testHandleErrorForTask();
    void testClearErrors();
    void testClearErrorsForTask();
    void testMaxStoredErrors();
    void testCanRecover();
    void testSetRecoveryHandler();
    void testShowDialogs();
    void testAutoRecover();
    void testErrorOccurredSignal();
    void testErrorsClearedSignal();
    void testSingleton();
};

#endif
