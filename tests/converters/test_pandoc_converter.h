#ifndef TEST_PANDOC_CONVERTER_H
#define TEST_PANDOC_CONVERTER_H

#include <QObject>
#include <QString>

class TestPandocConverter : public QObject {
    Q_OBJECT
public:
    explicit TestPandocConverter(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void testSupportedInputFormats();
    void testSupportedOutputFormats();
    void testName();
    void testIsConversionSupported();
    void testSetPandocPath();
    void testGetPandocFormat();
    void testCheckPandocAvailable();
    void testGetPandocVersion();
    void testProgressChangedSignal();
    void testStatusChangedSignal();
    void testConversionFinishedSignal();
    void testErrorOccurredSignal();
    void testConvertWithoutPandoc();
    void testConvertWithInvalidInput();
    void testConvertWithEmptyInput();
private:
    bool checkPandocAvailable();
    bool m_pandocAvailable;
};

#endif
