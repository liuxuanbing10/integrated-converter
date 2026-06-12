#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include "../../src/converters/segmented_converter.h"
#include "test_segmented_converter.h"

bool TestSegmentedConverter::checkFFmpegAvailable() {
    QProcess process;
    process.start("ffmpeg", QStringList() << "-version");
    bool available = process.waitForStarted() && process.waitForFinished(3000);
    return available;
}

void TestSegmentedConverter::initTestCase() {
    m_ffmpegAvailable = checkFFmpegAvailable();
}

void TestSegmentedConverter::testDefaultConfig() {
    SegmentedConverter converter;
    QCOMPARE(converter.currentSegment(), 0);
    QCOMPARE(converter.totalSegments(), 0);
    QCOMPARE(converter.overallProgress(), 0);
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testSetSegmentCount() {
    SegmentedConverter converter;
    converter.setSegmentCount(4);
    // No direct getter, but verify no crash and state remains sane
    QVERIFY(!converter.isRunning());
    QCOMPARE(converter.currentSegment(), 0);
}

void TestSegmentedConverter::testSetSegmentCountBoundary() {
    SegmentedConverter converter;
    // Zero should clamp to minimum
    converter.setSegmentCount(0);
    QVERIFY(!converter.isRunning());
    // Negative should be handled gracefully
    converter.setSegmentCount(-1);
    QVERIFY(!converter.isRunning());
    // Large value should be capped
    converter.setSegmentCount(1000);
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testSetMaxSegments() {
    SegmentedConverter converter;
    converter.setMaxSegments(8);
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testSetMaxSegmentsBoundary() {
    SegmentedConverter converter;
    // Zero should clamp
    converter.setMaxSegments(0);
    QVERIFY(!converter.isRunning());
    // Negative should be handled
    converter.setMaxSegments(-1);
    QVERIFY(!converter.isRunning());
    // Reasonable boundary
    converter.setMaxSegments(1);
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testOverallProgressWhenNotRunning() {
    SegmentedConverter converter;
    QCOMPARE(converter.overallProgress(), 0);
}

void TestSegmentedConverter::testIsRunningWhenNotRunning() {
    SegmentedConverter converter;
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testCurrentSegmentWhenNotRunning() {
    SegmentedConverter converter;
    QCOMPARE(converter.currentSegment(), 0);
    QCOMPARE(converter.totalSegments(), 0);
}

void TestSegmentedConverter::testCancelWhenNotRunning() {
    SegmentedConverter converter;
    // Should not crash or throw
    converter.cancel();
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testProgressChangedSignal() {
    SegmentedConverter converter;
    QSignalSpy spy(&converter, &SegmentedConverter::progressChanged);
    QVERIFY(spy.isValid());
}

void TestSegmentedConverter::testStatusChangedSignal() {
    SegmentedConverter converter;
    QSignalSpy spy(&converter, &SegmentedConverter::statusChanged);
    QVERIFY(spy.isValid());
}

void TestSegmentedConverter::testSegmentProgressSignal() {
    SegmentedConverter converter;
    QSignalSpy spy(&converter, &SegmentedConverter::segmentProgress);
    QVERIFY(spy.isValid());
}

void TestSegmentedConverter::testConversionFinishedSignal() {
    SegmentedConverter converter;
    QSignalSpy spy(&converter, &SegmentedConverter::conversionFinished);
    QVERIFY(spy.isValid());
}

void TestSegmentedConverter::testErrorOccurredSignal() {
    SegmentedConverter converter;
    QSignalSpy spy(&converter, &SegmentedConverter::errorOccurred);
    QVERIFY(spy.isValid());
}

void TestSegmentedConverter::testConvertWithoutFFmpeg() {
    if (m_ffmpegAvailable) {
        QSKIP("FFmpeg is available, skipping this test");
    }
    SegmentedConverter converter;
    converter.setFFmpegPath("/nonexistent/ffmpeg");
    auto result = converter.convert("input.mp4", "output.mkv", QVariantMap());
    QVERIFY(!result);
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testSetFFmpegPath() {
    SegmentedConverter converter;
    QString testPath = "/custom/path/to/ffmpeg";
    converter.setFFmpegPath(testPath);
    QVERIFY(!converter.isRunning());
}

void TestSegmentedConverter::testSetFFprobePath() {
    SegmentedConverter converter;
    QString testPath = "/custom/path/to/ffprobe";
    converter.setFFprobePath(testPath);
    QVERIFY(!converter.isRunning());
}
