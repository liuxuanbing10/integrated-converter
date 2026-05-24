#ifndef TEST_FFMPEG_CONVERTER_H
#define TEST_FFMPEG_CONVERTER_H

#include <QObject>
#include <QString>

class TestFFmpegConverter : public QObject {
    Q_OBJECT
public:
    explicit TestFFmpegConverter(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void testSupportedInputFormats();
    void testSupportedOutputFormats();
    void testName();
    void testIsConversionSupported();
    void testSetFFmpegPath();
    void testSetFFprobePath();
    void testFormatRegistry();
    void testIsRunning();
    void testProgressChangedSignal();
    void testStatusChangedSignal();
    void testConversionFinishedSignal();
    void testErrorOccurredSignal();
    void testConvertWithoutFFmpeg();
    void testConvertWithInvalidInput();
    void testGetMediaInfoWithoutFFmpeg();
    void testGetDurationWithoutFFmpeg();
    void testCancel();
    void testSpeedMetrics();
private:
    bool checkFFmpegAvailable();
    bool m_ffmpegAvailable;
};

#endif
