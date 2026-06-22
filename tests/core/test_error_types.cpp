#include <QTest>
#include <optional>
#include "../../src/core/error_types.h"
#include "test_error_types.h"

void TestErrorTypes::testDefaultConstructor() {
    ErrorInfo error;
    QCOMPARE(error.code, ErrorCode::Unknown);
    QVERIFY(error.message.isEmpty());
    QVERIFY(error.timestamp.isValid());
    QVERIFY(!error.isValid());
}

void TestErrorTypes::testParameterizedConstructor() {
    ErrorInfo error(ErrorCode::ConversionFailed, "Something went wrong");
    QCOMPARE(error.code, ErrorCode::ConversionFailed);
    QCOMPARE(error.message, QString("Something went wrong"));
    QVERIFY(error.timestamp.isValid());
    QVERIFY(error.isValid());
}

void TestErrorTypes::testIsValid() {
    ErrorInfo empty;
    QVERIFY(!empty.isValid());

    ErrorInfo byCode(ErrorCode::ConversionFailed, "");
    // code != Unknown -> should be valid
    QVERIFY(byCode.isValid());

    ErrorInfo byMsg(ErrorCode::Unknown, "has message");
    // isEmpty() is false for message, so should be valid
    QVERIFY(byMsg.isValid());
}

void TestErrorTypes::testFullMessage() {
    ErrorInfo withoutDetails(ErrorCode::Unknown, "simple message");
    QCOMPARE(withoutDetails.fullMessage(), QString("simple message"));

    ErrorInfo withDetails(ErrorCode::Unknown, "main message");
    withDetails.details = "detailed info";
    QCOMPARE(withDetails.fullMessage(), QString("main message\ndetailed info"));
}

void TestErrorTypes::testErrorCodeToString() {
    QCOMPARE(ErrorTypes::errorCodeToString(ErrorCode::Unknown), QString("Unknown"));
    QCOMPARE(ErrorTypes::errorCodeToString(ErrorCode::FileNotFound), QString("FileNotFound"));
    QCOMPARE(ErrorTypes::errorCodeToString(ErrorCode::ConversionFailed), QString("ConversionFailed"));
    QCOMPARE(ErrorTypes::errorCodeToString(ErrorCode::TaskCancelled), QString("TaskCancelled"));
    QCOMPARE(ErrorTypes::errorCodeToString(ErrorCode::ProcessCrashed), QString("ProcessCrashed"));
}

void TestErrorTypes::testStringToErrorCode() {
    QCOMPARE(ErrorTypes::stringToErrorCode("Unknown"), ErrorCode::Unknown);
    QCOMPARE(ErrorTypes::stringToErrorCode("FileNotFound"), ErrorCode::FileNotFound);
    QCOMPARE(ErrorTypes::stringToErrorCode("ConversionFailed"), ErrorCode::ConversionFailed);
    QCOMPARE(ErrorTypes::stringToErrorCode("TaskCancelled"), ErrorCode::TaskCancelled);
    QCOMPARE(ErrorTypes::stringToErrorCode("ProcessCrashed"), ErrorCode::ProcessCrashed);
    // unknown string -> Unknown
    QCOMPARE(ErrorTypes::stringToErrorCode("NonExistent"), ErrorCode::Unknown);
}

void TestErrorTypes::testRoundTripAllCodes() {
    // Verify all ErrorCode values survive a round-trip through errorCodeToString + stringToErrorCode
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::Unknown)), ErrorCode::Unknown);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::InvalidParameter)), ErrorCode::InvalidParameter);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::FileNotFound)), ErrorCode::FileNotFound);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::PermissionDenied)), ErrorCode::PermissionDenied);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::DiskSpaceInsufficient)), ErrorCode::DiskSpaceInsufficient);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::ConverterNotFound)), ErrorCode::ConverterNotFound);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::ConverterNotAvailable)), ErrorCode::ConverterNotAvailable);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::UnsupportedFormat)), ErrorCode::UnsupportedFormat);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::ConversionFailed)), ErrorCode::ConversionFailed);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::TaskCancelled)), ErrorCode::TaskCancelled);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::TaskTimeout)), ErrorCode::TaskTimeout);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::ProcessCrashed)), ErrorCode::ProcessCrashed);
    QCOMPARE(ErrorTypes::stringToErrorCode(ErrorTypes::errorCodeToString(ErrorCode::ProcessFailedToStart)), ErrorCode::ProcessFailedToStart);
}

void TestErrorTypes::testDefaultSuggestion() {
    // All error codes should have a non-empty default suggestion
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::Unknown).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::FileNotFound).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::PermissionDenied).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::DiskSpaceInsufficient).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::ConverterNotFound).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::ConverterNotAvailable).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::UnsupportedFormat).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::ConversionFailed).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::TaskCancelled).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::TaskTimeout).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::ProcessCrashed).isEmpty());
    QVERIFY(!ErrorTypes::defaultSuggestion(ErrorCode::ProcessFailedToStart).isEmpty());
}

void TestErrorTypes::testIsRecoverable() {
    // Recoverable errors
    QVERIFY(ErrorTypes::isRecoverable(ErrorCode::FileNotFound));
    QVERIFY(ErrorTypes::isRecoverable(ErrorCode::PermissionDenied));
    QVERIFY(ErrorTypes::isRecoverable(ErrorCode::DiskSpaceInsufficient));
    QVERIFY(ErrorTypes::isRecoverable(ErrorCode::ConverterNotAvailable));
    QVERIFY(ErrorTypes::isRecoverable(ErrorCode::ConversionFailed));
    QVERIFY(ErrorTypes::isRecoverable(ErrorCode::TaskTimeout));

    // Non-recoverable errors
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::Unknown));
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::InvalidParameter));
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::ConverterNotFound));
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::UnsupportedFormat));
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::TaskCancelled));
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::ProcessCrashed));
    QVERIFY(!ErrorTypes::isRecoverable(ErrorCode::ProcessFailedToStart));
}

void TestErrorTypes::testCreateError() {
    ErrorInfo error = ErrorTypes::createError(ErrorCode::InvalidParameter, "bad param",
                                              "test_context", "task-42");
    QCOMPARE(error.code, ErrorCode::InvalidParameter);
    QCOMPARE(error.message, QString("bad param"));
    QCOMPARE(error.context, QString("test_context"));
    QCOMPARE(error.taskId, QString("task-42"));
    QVERIFY(!error.suggestion.isEmpty());
    QCOMPARE(error.recoverable, false);
    QVERIFY(error.timestamp.isValid());
}

void TestErrorTypes::testCreateFileNotFoundError() {
    ErrorInfo error = ErrorTypes::createFileNotFoundError("/path/to/missing.txt", "test");
    QCOMPARE(error.code, ErrorCode::FileNotFound);
    QVERIFY(!error.message.isEmpty());
    QVERIFY(error.details.contains("/path/to/missing.txt"));
    QCOMPARE(error.inputFile, QString("/path/to/missing.txt"));
    QCOMPARE(error.context, QString("test"));
    QVERIFY(error.recoverable);
    QVERIFY(!error.suggestion.isEmpty());
}

void TestErrorTypes::testCreateConversionFailedError() {
    ErrorInfo error = ErrorTypes::createConversionFailedError("ffmpeg crashed", "FFmpeg", "convert");
    QCOMPARE(error.code, ErrorCode::ConversionFailed);
    QCOMPARE(error.details, QString("ffmpeg crashed"));
    QCOMPARE(error.converterName, QString("FFmpeg"));
    QCOMPARE(error.context, QString("convert"));
    QVERIFY(error.recoverable);
}

void TestErrorTypes::testCreateProcessError() {
    // Process crash
    ErrorInfo crash = ErrorTypes::createProcessError(ErrorCode::ProcessCrashed, "ffmpeg",
                                                     "segfault", "convert");
    QCOMPARE(crash.code, ErrorCode::ProcessCrashed);
    QVERIFY(!crash.message.isEmpty());
    QVERIFY(crash.details.contains("ffmpeg"));
    QCOMPARE(crash.context, QString("convert"));
    QVERIFY(!crash.recoverable);

    // Process failed to start
    ErrorInfo noStart = ErrorTypes::createProcessError(ErrorCode::ProcessFailedToStart,
                                                       "pandoc", "", "test");
    QCOMPARE(noStart.code, ErrorCode::ProcessFailedToStart);
    QVERIFY(!noStart.message.isEmpty());
    QVERIFY(!noStart.recoverable);
}
