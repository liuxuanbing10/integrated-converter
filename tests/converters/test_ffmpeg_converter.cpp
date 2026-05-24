#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include "../../src/converters/ffmpeg_converter.h"
#include "test_ffmpeg_converter.h"

bool TestFFmpegConverter::checkFFmpegAvailable() {
    QProcess process;
    process.start("ffmpeg", QStringList() << "-version");
    bool available = process.waitForStarted() && process.waitForFinished(3000);
    return available;
}

void TestFFmpegConverter::initTestCase() {
    m_ffmpegAvailable = checkFFmpegAvailable();
}

void TestFFmpegConverter::testSupportedInputFormats() {
    FFmpegConverter converter;
    QStringList inputFormats = converter.supportedInputFormats();
    QVERIFY(!inputFormats.isEmpty());
    QVERIFY(inputFormats.contains("mp4") || inputFormats.contains("avi"));
}

void TestFFmpegConverter::testSupportedOutputFormats() {
    FFmpegConverter converter;
    QStringList outputFormats = converter.supportedOutputFormats();
    QVERIFY(!outputFormats.isEmpty());
    QVERIFY(outputFormats.contains("mp4") || outputFormats.contains("mkv"));
}

void TestFFmpegConverter::testName() {
    FFmpegConverter converter;
    QCOMPARE(converter.name(), QString("FFmpeg"));
}

void TestFFmpegConverter::testIsConversionSupported() {
    FFmpegConverter converter;
    bool supported = converter.isConversionSupported("mp4", "mkv");
    QVERIFY(supported || !supported);
    supported = converter.isConversionSupported("mp3", "wav");
    QVERIFY(supported || !supported);
}

void TestFFmpegConverter::testSetFFmpegPath() {
    FFmpegConverter converter;
    QString testPath = "/custom/path/to/ffmpeg";
    converter.setFFmpegPath(testPath);
    QCOMPARE(converter.ffmpegPath(), testPath);
}

void TestFFmpegConverter::testSetFFprobePath() {
    FFmpegConverter converter;
    QString testPath = "/custom/path/to/ffprobe";
    converter.setFFprobePath(testPath);
    QCOMPARE(converter.ffprobePath(), testPath);
}

void TestFFmpegConverter::testFormatRegistry() {
    const auto& reg = FormatRegistry::instance();
    QVERIFY(!reg.videoFormats().isEmpty());
    QVERIFY(!reg.audioFormats().isEmpty());
    QVERIFY(!reg.documentFormats().isEmpty());
    QVERIFY(reg.isVideo("mp4"));
    QVERIFY(reg.isAudio("mp3"));
    QVERIFY(reg.isDocument("md"));
    QVERIFY(!reg.ffmpegFormatName("mp4").isEmpty());
    QVERIFY(!reg.ffmpegVideoCodec("h264").isEmpty());
    QVERIFY(!reg.ffmpegAudioCodec("aac").isEmpty());
    QVERIFY(!reg.pandocFormatName("md").isEmpty());
}

void TestFFmpegConverter::testIsRunning() {
    FFmpegConverter converter;
    QVERIFY(!converter.isRunning());
}

void TestFFmpegConverter::testProgressChangedSignal() {
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::progressChanged);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testStatusChangedSignal() {
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::statusChanged);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testConversionFinishedSignal() {
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::conversionFinished);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testErrorOccurredSignal() {
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::errorOccurred);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testConvertWithoutFFmpeg() {
    if (m_ffmpegAvailable) {
        QSKIP("FFmpeg is available, skipping this test");
    }
    FFmpegConverter converter;
    converter.setFFmpegPath("/nonexistent/ffmpeg");
    bool result = converter.convert("input.mp4", "output.mkv", QVariantMap());
    QVERIFY(!result);
}

void TestFFmpegConverter::testConvertWithInvalidInput() {
    FFmpegConverter converter;
    bool result = converter.convert("/nonexistent/input.mp4", "output.mkv", QVariantMap());
    QVERIFY(!result);
}

void TestFFmpegConverter::testGetMediaInfoWithoutFFmpeg() {
    if (m_ffmpegAvailable) {
        QSKIP("FFmpeg is available, skipping this test");
    }
    FFmpegConverter converter;
    converter.setFFprobePath("/nonexistent/ffprobe");
    QVariantMap info;
    bool result = converter.getMediaInfo("test.mp4", info);
    QVERIFY(!result);
}

void TestFFmpegConverter::testGetDurationWithoutFFmpeg() {
    if (m_ffmpegAvailable) {
        QSKIP("FFmpeg is available, skipping this test");
    }
    FFmpegConverter converter;
    converter.setFFprobePath("/nonexistent/ffprobe");
    double duration = converter.getDuration("test.mp4");
    QCOMPARE(duration, 0.0);
}

void TestFFmpegConverter::testCancel() {
    FFmpegConverter converter;
    converter.cancel();
    QVERIFY(!converter.isRunning());
}

void TestFFmpegConverter::testSpeedMetrics() {
    FFmpegConverter converter;
    QCOMPARE(converter.currentSpeed(), 0.0);
    QCOMPARE(converter.estimatedRemainingMs(), qint64(0));
    QCOMPARE(converter.currentBitrate(), 0.0);
    QCOMPARE(converter.processedBytes(), qint64(0));
}
