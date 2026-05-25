#ifndef TEST_IMAGEMAGICK_CONVERTER_H
#define TEST_IMAGEMAGICK_CONVERTER_H

#include <QObject>
#include <QString>

class TestImageMagickConverter : public QObject {
    Q_OBJECT
public:
    explicit TestImageMagickConverter(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void testSupportedInputFormats();
    void testSupportedOutputFormats();
    void testName();
    void testIsConversionSupported();
    void testSetMagickPath();
    void testFormatRegistry();
    void testIsRunning();
    void testProgressChangedSignal();
    void testStatusChangedSignal();
    void testConversionFinishedSignal();
    void testErrorOccurredSignal();
    void testConvertWithoutImageMagick();
    void testConvertWithInvalidInput();
    void testCancel();
private:
    bool checkImageMagickAvailable();
    bool m_magickAvailable;
};

#endif
