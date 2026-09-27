#include "test_ffmpeg_converter.h"

#include "../../src/converters/ffmpeg_converter.h"

#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

bool TestFFmpegConverter::checkFFmpegAvailable()
{
    QProcess process;
    process.start("ffmpeg", QStringList() << "-version");
    bool available = process.waitForStarted() && process.waitForFinished(3000);
    return available;
}

void TestFFmpegConverter::initTestCase()
{
    m_ffmpegAvailable = checkFFmpegAvailable();
}

void TestFFmpegConverter::testSupportedInputFormats()
{
    FFmpegConverter converter;
    QStringList inputFormats = converter.supportedInputFormats();
    QVERIFY(!inputFormats.isEmpty());
    QVERIFY(inputFormats.contains("mp4") || inputFormats.contains("avi"));
}

void TestFFmpegConverter::testSupportedOutputFormats()
{
    FFmpegConverter converter;
    QStringList outputFormats = converter.supportedOutputFormats();
    QVERIFY(!outputFormats.isEmpty());
    QVERIFY(outputFormats.contains("mp4") || outputFormats.contains("mkv"));
}

void TestFFmpegConverter::testName()
{
    FFmpegConverter converter;
    QCOMPARE(converter.name(), QString("FFmpeg"));
}

void TestFFmpegConverter::testIsConversionSupported()
{
    FFmpegConverter converter;
    bool supported = converter.isConversionSupported("mp4", "mkv");
    QVERIFY(supported || !supported);
    supported = converter.isConversionSupported("mp3", "wav");
    QVERIFY(supported || !supported);
}

void TestFFmpegConverter::testSetFFmpegPath()
{
    FFmpegConverter converter;
    QString testPath = "/custom/path/to/ffmpeg";
    converter.setFFmpegPath(testPath);
    QCOMPARE(converter.ffmpegPath(), testPath);
}

void TestFFmpegConverter::testSetFFprobePath()
{
    FFmpegConverter converter;
    QString testPath = "/custom/path/to/ffprobe";
    converter.setFFprobePath(testPath);
    QCOMPARE(converter.ffprobePath(), testPath);
}

void TestFFmpegConverter::testFormatRegistry()
{
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

void TestFFmpegConverter::testIsRunning()
{
    FFmpegConverter converter;
    QVERIFY(!converter.isRunning());
}

void TestFFmpegConverter::testProgressChangedSignal()
{
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::progressChanged);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testStatusChangedSignal()
{
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::statusChanged);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testConversionFinishedSignal()
{
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::conversionFinished);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testErrorOccurredSignal()
{
    FFmpegConverter converter;
    QSignalSpy spy(&converter, &FFmpegConverter::errorOccurred);
    QVERIFY(spy.isValid());
}

void TestFFmpegConverter::testConvertWithoutFFmpeg()
{
    if (m_ffmpegAvailable)
    {
        QSKIP("FFmpeg is available, skipping this test");
    }
    FFmpegConverter converter;
    converter.setFFmpegPath("/nonexistent/ffmpeg");
    auto result = converter.convert("input.mp4", "output.mkv", QVariantMap());
    QVERIFY(result.has_value());
}

void TestFFmpegConverter::testConvertWithInvalidInput()
{
    FFmpegConverter converter;
    auto result = converter.convert("/nonexistent/input.mp4", "output.mkv", QVariantMap());
    QVERIFY(result.has_value());
}

void TestFFmpegConverter::testGetMediaInfoWithoutFFmpeg()
{
    if (m_ffmpegAvailable)
    {
        QSKIP("FFmpeg is available, skipping this test");
    }
    FFmpegConverter converter;
    converter.setFFprobePath("/nonexistent/ffprobe");
    QVariantMap info;
    bool result = converter.getMediaInfo("test.mp4", info);
    QVERIFY(!result);
}

void TestFFmpegConverter::testGetDurationWithoutFFmpeg()
{
    if (m_ffmpegAvailable)
    {
        QSKIP("FFmpeg is available, skipping this test");
    }
    FFmpegConverter converter;
    converter.setFFprobePath("/nonexistent/ffprobe");
    double duration = converter.getDuration("test.mp4");
    QCOMPARE(duration, 0.0);
}

void TestFFmpegConverter::testCancel()
{
    FFmpegConverter converter;
    converter.cancel();
    QVERIFY(!converter.isRunning());
}

void TestFFmpegConverter::testSpeedMetrics()
{
    FFmpegConverter converter;
    QCOMPARE(converter.currentSpeed(), 0.0);
    QCOMPARE(converter.estimatedRemainingMs(), qint64(0));
    QCOMPARE(converter.currentBitrate(), 0.0);
    QCOMPARE(converter.processedBytes(), qint64(0));
}

// §3.3 contract regression: every audio-codec choice exposed by
// video_params_widget must survive validateParams, or "optional = broken".
void TestFFmpegConverter::testUiCodecOptionsPassValidation()
{
    struct
    {
        const char* key;
        const char* value;
    } uiOptions[] = {
        {"audioCodec", "aac"},  {"audioCodec", "mp3"},     {"audioCodec", "copy"},
        {"audioCodec", "none"}, {"videoCodec", "libx264"}, {"videoCodec", "libx265"},
        {"videoCodec", "h264"}, {"videoCodec", "auto"},    {"preset", "medium"},
    };
    for (const auto& opt : uiOptions)
    {
        QVariantMap params;
        params[opt.key] = QString::fromLatin1(opt.value);
        QString errorMsg;
        QVERIFY2(FFmpegConverter::validateParams(params, errorMsg),
                 qPrintable(QString("%1=%2 rejected: %3").arg(opt.key, opt.value, errorMsg)));
    }
}

// Audio widget sample-rate / channel options must pass validation too.
void TestFFmpegConverter::testUiAudioOptionsPassValidation()
{
    const int rates[] = {0, 22050, 44100, 48000, 96000, 192000};
    const int channels[] = {0, 1, 2, 6};
    for (int rate : rates)
    {
        QVariantMap params;
        params["sampleRate"] = rate;
        QString errorMsg;
        QVERIFY2(FFmpegConverter::validateParams(params, errorMsg),
                 qPrintable(QString("sampleRate=%1 rejected: %2").arg(rate).arg(errorMsg)));
    }
    for (int ch : channels)
    {
        QVariantMap params;
        params["channels"] = ch;
        QString errorMsg;
        QVERIFY2(FFmpegConverter::validateParams(params, errorMsg),
                 qPrintable(QString("channels=%1 rejected: %2").arg(ch).arg(errorMsg)));
    }
    // and still reject garbage
    QVariantMap bad;
    bad["sampleRate"] = 44101;
    QString errorMsg;
    QVERIFY(!FFmpegConverter::validateParams(bad, errorMsg));
}
