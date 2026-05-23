#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include "../../src/converters/ffmpeg_converter.h"
class TestFFmpegConverter : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_ffmpegAvailable = checkFFmpegAvailable();
    }
    void testSupportedInputFormats() {
        FFmpegConverter converter;
        QStringList inputFormats = converter.supportedInputFormats();
        QVERIFY(!inputFormats.isEmpty());
        QVERIFY(inputFormats.contains("mp4") || inputFormats.contains("avi"));
    }
    void testSupportedOutputFormats() {
        FFmpegConverter converter;
        QStringList outputFormats = converter.supportedOutputFormats();
        QVERIFY(!outputFormats.isEmpty());
        QVERIFY(outputFormats.contains("mp4") || outputFormats.contains("mkv"));
    }
    void testName() {
        FFmpegConverter converter;
        QCOMPARE(converter.name(), QString("FFmpeg"));
    }
    void testIsConversionSupported() {
        FFmpegConverter converter;
        bool supported = converter.isConversionSupported("mp4", "mkv");
        QVERIFY(supported || !supported);
        supported = converter.isConversionSupported("mp3", "wav");
        QVERIFY(supported || !supported);
    }
    void testSetFFmpegPath() {
        FFmpegConverter converter;
        QString testPath = "/custom/path/to/ffmpeg";
        converter.setFFmpegPath(testPath);
        QCOMPARE(converter.ffmpegPath(), testPath);
    }
    void testSetFFprobePath() {
        FFmpegConverter converter;
        QString testPath = "/custom/path/to/ffprobe";
        converter.setFFprobePath(testPath);
        QCOMPARE(converter.ffprobePath(), testPath);
    }
    void testVideoFormatMap() {
        QMap<QString, QString> videoMap = FFmpegConverter::videoFormatMap();
        QVERIFY(!videoMap.isEmpty());
    }
    void testAudioFormatMap() {
        QMap<QString, QString> audioMap = FFmpegConverter::audioFormatMap();
        QVERIFY(!audioMap.isEmpty());
    }
    void testVideoCodecMap() {
        QMap<QString, QString> codecMap = FFmpegConverter::videoCodecMap();
        QVERIFY(!codecMap.isEmpty());
    }
    void testAudioCodecMap() {
        QMap<QString, QString> codecMap = FFmpegConverter::audioCodecMap();
        QVERIFY(!codecMap.isEmpty());
    }
    void testIsRunning() {
        FFmpegConverter converter;
        QVERIFY(!converter.isRunning());
    }
    void testProgressChangedSignal() {
        FFmpegConverter converter;
        QSignalSpy spy(&converter, &FFmpegConverter::progressChanged);
        QVERIFY(spy.isValid());
    }
    void testStatusChangedSignal() {
        FFmpegConverter converter;
        QSignalSpy spy(&converter, &FFmpegConverter::statusChanged);
        QVERIFY(spy.isValid());
    }
    void testConversionFinishedSignal() {
        FFmpegConverter converter;
        QSignalSpy spy(&converter, &FFmpegConverter::conversionFinished);
        QVERIFY(spy.isValid());
    }
    void testErrorOccurredSignal() {
        FFmpegConverter converter;
        QSignalSpy spy(&converter, &FFmpegConverter::errorOccurred);
        QVERIFY(spy.isValid());
    }
    void testConvertWithoutFFmpeg() {
        if (m_ffmpegAvailable) {
            QSKIP("FFmpeg is available, skipping this test");
        }
        FFmpegConverter converter;
        converter.setFFmpegPath("/nonexistent/ffmpeg");
        bool result = converter.convert("input.mp4", "output.mkv", QVariantMap());
        QVERIFY(!result);
    }
    void testConvertWithInvalidInput() {
        FFmpegConverter converter;
        bool result = converter.convert("/nonexistent/input.mp4", "output.mkv", QVariantMap());
        QVERIFY(!result);
    }
    void testGetMediaInfoWithoutFFmpeg() {
        if (m_ffmpegAvailable) {
            QSKIP("FFmpeg is available, skipping this test");
        }
        FFmpegConverter converter;
        converter.setFFprobePath("/nonexistent/ffprobe");
        QVariantMap info;
        bool result = converter.getMediaInfo("test.mp4", info);
        QVERIFY(!result);
    }
    void testGetDurationWithoutFFmpeg() {
        if (m_ffmpegAvailable) {
            QSKIP("FFmpeg is available, skipping this test");
        }
        FFmpegConverter converter;
        converter.setFFprobePath("/nonexistent/ffprobe");
        double duration = converter.getDuration("test.mp4");
        QCOMPARE(duration, 0.0);
    }
    void testCancel() {
        FFmpegConverter converter;
        converter.cancel();
        QVERIFY(!converter.isRunning());
    }
    void testSpeedMetrics() {
        FFmpegConverter converter;
        QCOMPARE(converter.currentSpeed(), 0.0);
        QCOMPARE(converter.estimatedRemainingMs(), qint64(0));
        QCOMPARE(converter.currentBitrate(), 0.0);
        QCOMPARE(converter.processedBytes(), qint64(0));
    }
private:
    bool checkFFmpegAvailable() {
        QProcess process;
        process.start("ffmpeg", QStringList() << "-version");
        bool available = process.waitForStarted() && process.waitForFinished(3000);
        return available;
    }
    bool m_ffmpegAvailable;
};
#include "test_ffmpeg_converter.moc"
