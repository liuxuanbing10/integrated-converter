#ifndef TEST_ERROR_TYPES_H
#define TEST_ERROR_TYPES_H

#include <QObject>

class TestErrorTypes : public QObject {
    Q_OBJECT
public:
    explicit TestErrorTypes(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    // ErrorInfo construction
    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testIsValid();
    void testFullMessage();

    // ErrorTypes factory functions
    void testErrorCodeToString();
    void testStringToErrorCode();
    void testRoundTripAllCodes();
    void testDefaultSuggestion();
    void testIsRecoverable();

    void testCreateError();
    void testCreateFileNotFoundError();
    void testCreateConversionFailedError();
    void testCreateProcessError();
};

#endif // TEST_ERROR_TYPES_H
