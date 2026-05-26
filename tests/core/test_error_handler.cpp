#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include "../../src/core/error_handler.h"
#include "../../src/core/error_types.h"
#include "test_error_handler.h"

void TestErrorHandler::initTestCase() {
    ErrorHandler::instance()->clearErrors();
}

void TestErrorHandler::cleanupTestCase() {
    ErrorHandler::instance()->clearErrors();
}

void TestErrorHandler::init() {
    ErrorHandler::instance()->clearErrors();
}

void TestErrorHandler::testHandleError() {
    ErrorHandler* handler = ErrorHandler::instance();
    ErrorInfo error(ErrorCode::ConversionFailed, "Test error message");
    error.details = "Test error details";
    handler->handleError(error);
    QCOMPARE(handler->errorCount(), 1);
    ErrorInfo retrieved = handler->lastError();
    QCOMPARE(retrieved.message, QString("Test error message"));
}

void TestErrorHandler::testHandleMultipleErrors() {
    ErrorHandler* handler = ErrorHandler::instance();
    for (int i = 0; i < 5; ++i) {
        ErrorInfo error(ErrorCode::ConversionFailed, QString("Error %1").arg(i));
        handler->handleError(error);
    }
    QCOMPARE(handler->errorCount(), 5);
}

void TestErrorHandler::testHandleException() {
    ErrorHandler* handler = ErrorHandler::instance();
    handler->handleException(std::runtime_error("Test exception"), "test_context");
    QCOMPARE(handler->errorCount(), 1);
}

void TestErrorHandler::testHandleErrorForTask() {
    ErrorHandler* handler = ErrorHandler::instance();
    ErrorInfo error(ErrorCode::ConversionFailed, "Task error");
    handler->handleErrorForTask(error, "task-1");
    QCOMPARE(handler->errorCount(), 1);
    QCOMPARE(handler->errorCountForTask("task-1"), 1);
    QCOMPARE(handler->errorCountForTask("non-existent"), 0);
}

void TestErrorHandler::testClearErrors() {
    ErrorHandler* handler = ErrorHandler::instance();
    ErrorInfo error(ErrorCode::ConversionFailed, "To be cleared");
    handler->handleError(error);
    QVERIFY(handler->errorCount() > 0);
    handler->clearErrors();
    QCOMPARE(handler->errorCount(), 0);
}

void TestErrorHandler::testClearErrorsForTask() {
    ErrorHandler* handler = ErrorHandler::instance();
    ErrorInfo error(ErrorCode::ConversionFailed, "Task error to clear");
    handler->handleErrorForTask(error, "task-clear");
    QVERIFY(handler->errorCountForTask("task-clear") > 0);
    handler->clearErrorsForTask("task-clear");
    QCOMPARE(handler->errorCountForTask("task-clear"), 0);
}

void TestErrorHandler::testMaxStoredErrors() {
    ErrorHandler* handler = ErrorHandler::instance();
    handler->setMaxStoredErrors(3);
    for (int i = 0; i < 10; ++i) {
        ErrorInfo error(ErrorCode::ConversionFailed, QString("Error %1").arg(i));
        handler->handleError(error);
    }
    QCOMPARE(handler->errorCount(), 3);
}

void TestErrorHandler::testCanRecover() {
    ErrorHandler* handler = ErrorHandler::instance();
    // canRecover requires both recoverable==true AND a registered handler
    handler->setRecoveryHandler(ErrorCode::ProcessCrashed, [](const ErrorInfo&){});
    ErrorInfo recoverable(ErrorCode::ProcessCrashed, "Recoverable");
    recoverable.recoverable = true;
    QVERIFY(handler->canRecover(recoverable));
    ErrorInfo notRecoverable(ErrorCode::ConversionFailed, "Not recoverable");
    notRecoverable.recoverable = false;
    QVERIFY(!handler->canRecover(notRecoverable));
}

void TestErrorHandler::testSetRecoveryHandler() {
    ErrorHandler* handler = ErrorHandler::instance();
    bool recoveryCalled = false;
    handler->setRecoveryHandler(ErrorCode::ProcessCrashed, [&recoveryCalled](const ErrorInfo&) {
        recoveryCalled = true;
    });
    ErrorInfo error(ErrorCode::ProcessCrashed, "Recover me");
    error.recoverable = true;
    handler->handleError(error);
    // handleError does NOT automatically attempt recovery (that requires
    // autoRecover=true + timer delay); explicitly call attemptRecovery instead.
    QVERIFY(handler->canRecover(error));
    handler->attemptRecovery(error);
    QVERIFY(recoveryCalled);
}

void TestErrorHandler::testShowDialogs() {
    ErrorHandler* handler = ErrorHandler::instance();
    bool orig = handler->showDialogs();
    handler->setShowDialogs(false);
    QVERIFY(!handler->showDialogs());
    handler->setShowDialogs(true);
    QVERIFY(handler->showDialogs());
    handler->setShowDialogs(orig);
}

void TestErrorHandler::testAutoRecover() {
    ErrorHandler* handler = ErrorHandler::instance();
    handler->setAutoRecover(true);
    QVERIFY(handler->autoRecover());
    handler->setAutoRecover(false);
    QVERIFY(!handler->autoRecover());
}

void TestErrorHandler::testErrorOccurredSignal() {
    ErrorHandler* handler = ErrorHandler::instance();
    QSignalSpy spy(handler, &ErrorHandler::errorOccurred);
    ErrorInfo error(ErrorCode::ConversionFailed, "Signal test");
    handler->handleError(error);
    QCOMPARE(spy.count(), 1);
}

void TestErrorHandler::testErrorsClearedSignal() {
    ErrorHandler* handler = ErrorHandler::instance();
    QSignalSpy spy(handler, &ErrorHandler::errorsCleared);
    ErrorInfo error(ErrorCode::ConversionFailed, "Clear test");
    handler->handleError(error);
    handler->clearErrors();
    QCOMPARE(spy.count(), 1);
}

void TestErrorHandler::testSingleton() {
    ErrorHandler* instance1 = ErrorHandler::instance();
    ErrorHandler* instance2 = ErrorHandler::instance();
    QCOMPARE(instance1, instance2);
}

