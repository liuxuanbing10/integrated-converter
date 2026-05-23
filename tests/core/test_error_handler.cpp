#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include "../../src/core/error_handler.h"
#include "../../src/core/error_types.h"
class TestErrorHandler : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        ErrorHandler::instance()->clearErrors();
    }
    void cleanupTestCase() {
        ErrorHandler::instance()->clearErrors();
    }
    void init() {
        ErrorHandler::instance()->clearErrors();
        ErrorHandler::instance()->setShowDialogs(false);
        ErrorHandler::instance()->setAutoRecover(false);
    }
    void testHandleError() {
        ErrorHandler* eh = ErrorHandler::instance();
        ErrorInfo error(ErrorCode::FileNotFound, "Test file not found");
        eh->handleError(error);
        QCOMPARE(eh->errorCount(), 1);
        QVERIFY(eh->hasErrors());
        ErrorInfo lastError = eh->lastError();
        QCOMPARE(lastError.code, ErrorCode::FileNotFound);
        QCOMPARE(lastError.message, QString("Test file not found"));
    }
    void testHandleMultipleErrors() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->clearErrors();
        ErrorInfo error1(ErrorCode::FileNotFound, "Error 1");
        ErrorInfo error2(ErrorCode::PermissionDenied, "Error 2");
        ErrorInfo error3(ErrorCode::DiskSpaceInsufficient, "Error 3");
        eh->handleError(error1);
        eh->handleError(error2);
        eh->handleError(error3);
        QCOMPARE(eh->errorCount(), 3);
        QList<ErrorInfo> recentErrors = eh->recentErrors();
        QCOMPARE(recentErrors.size(), 3);
    }
    void testHandleException() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->clearErrors();
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            eh->handleException(e, "TestContext");
        }
        QVERIFY(eh->hasErrors());
        ErrorInfo lastError = eh->lastError();
        QVERIFY(lastError.message.contains("Test exception"));
    }
    void testHandleErrorForTask() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->clearErrors();
        ErrorInfo error(ErrorCode::ConversionFailed, "Conversion failed");
        QString taskId = "test-task-123";
        eh->handleErrorForTask(error, taskId);
        QCOMPARE(eh->errorCountForTask(taskId), 1);
        QList<ErrorInfo> taskErrors = eh->errorsForTask(taskId);
        QCOMPARE(taskErrors.size(), 1);
        QCOMPARE(taskErrors[0].code, ErrorCode::ConversionFailed);
    }
    void testClearErrors() {
        ErrorHandler* eh = ErrorHandler::instance();
        ErrorInfo error(ErrorCode::Unknown, "Test error");
        eh->handleError(error);
        QVERIFY(eh->hasErrors());
        eh->clearErrors();
        QVERIFY(!eh->hasErrors());
        QCOMPARE(eh->errorCount(), 0);
    }
    void testClearErrorsForTask() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->clearErrors();
        ErrorInfo error(ErrorCode::ConversionFailed, "Task error");
        QString taskId = "task-to-clear";
        eh->handleErrorForTask(error, taskId);
        QCOMPARE(eh->errorCountForTask(taskId), 1);
        eh->clearErrorsForTask(taskId);
        QCOMPARE(eh->errorCountForTask(taskId), 0);
    }
    void testMaxStoredErrors() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->clearErrors();
        eh->setMaxStoredErrors(5);
        for (int i = 0; i < 10; ++i) {
            ErrorInfo error(ErrorCode::Unknown, QString("Error %1").arg(i));
            eh->handleError(error);
        }
        QVERIFY(eh->errorCount() <= 5);
    }
    void testCanRecover() {
        ErrorHandler* eh = ErrorHandler::instance();
        ErrorInfo recoverableError(ErrorCode::FileNotFound, "File not found");
        recoverableError.recoverable = true;
        QVERIFY(eh->canRecover(recoverableError));
        ErrorInfo nonRecoverableError(ErrorCode::Unknown, "Unknown error");
        nonRecoverableError.recoverable = false;
        QVERIFY(!eh->canRecover(nonRecoverableError));
    }
    void testSetRecoveryHandler() {
        ErrorHandler* eh = ErrorHandler::instance();
        bool handlerCalled = false;
        auto handler = [&handlerCalled](const ErrorInfo& error) {
            handlerCalled = true;
            Q_UNUSED(error);
        };
        eh->setRecoveryHandler(ErrorCode::FileNotFound, handler);
        ErrorInfo error(ErrorCode::FileNotFound, "Test recovery");
        eh->attemptRecovery(error);
    }
    void testShowDialogs() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->setShowDialogs(true);
        QVERIFY(eh->showDialogs());
        eh->setShowDialogs(false);
        QVERIFY(!eh->showDialogs());
    }
    void testAutoRecover() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->setAutoRecover(true);
        QVERIFY(eh->autoRecover());
        eh->setAutoRecover(false);
        QVERIFY(!eh->autoRecover());
    }
    void testErrorOccurredSignal() {
        ErrorHandler* eh = ErrorHandler::instance();
        eh->clearErrors();
        QSignalSpy spy(eh, &ErrorHandler::errorOccurred);
        ErrorInfo error(ErrorCode::FileNotFound, "Signal test");
        eh->handleError(error);
        QCOMPARE(spy.count(), 1);
        ErrorInfo receivedError = spy.at(0).at(0).value<ErrorInfo>();
        QCOMPARE(receivedError.code, ErrorCode::FileNotFound);
    }
    void testErrorsClearedSignal() {
        ErrorHandler* eh = ErrorHandler::instance();
        ErrorInfo error(ErrorCode::Unknown, "Test");
        eh->handleError(error);
        QSignalSpy spy(eh, &ErrorHandler::errorsCleared);
        eh->clearErrors();
        QCOMPARE(spy.count(), 1);
    }
    void testSingleton() {
        ErrorHandler* instance1 = ErrorHandler::instance();
        ErrorHandler* instance2 = ErrorHandler::instance();
        QCOMPARE(instance1, instance2);
    }
};
#include "test_error_handler.moc"
