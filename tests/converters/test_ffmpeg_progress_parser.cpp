#include <QTest>
#include <QString>
#include "../../src/converters/ffmpeg_progress_parser.h"
#include "test_ffmpeg_progress_parser.h"

using Info = FfmpegProgressInfo;

void TestFfmpegProgressParser::testDefaultStructIsAllAbsent() {
    Info info;
    QVERIFY(!info.hasTime());
    QVERIFY(!info.hasSpeed());
    QVERIFY(!info.hasBitrate());
    QVERIFY(!info.hasSize());
    QCOMPARE(info.status, Info::Status::Unknown);
    QCOMPARE(info.timeMs, qint64(-1));
    QCOMPARE(info.speed, -1.0);
    QCOMPARE(info.bitrateKbps, -1.0);
    QCOMPARE(info.sizeBytes, qint64(-1));
}

void TestFfmpegProgressParser::testParseEmptyLine() {
    FfmpegProgressParser p;
    Info info = p.parseLine("");
    QVERIFY(!info.hasTime());
    QVERIFY(!info.hasSpeed());
    QVERIFY(!info.hasBitrate());
    QVERIFY(!info.hasSize());
    QCOMPARE(info.status, Info::Status::Unknown);
}

void TestFfmpegProgressParser::testParseBlankLine() {
    FfmpegProgressParser p;
    Info info = p.parseLine("   \t  ");
    QVERIFY(!info.hasTime());
    QVERIFY(!info.hasSpeed());
    QVERIFY(!info.hasBitrate());
    QVERIFY(!info.hasSize());
    QCOMPARE(info.status, Info::Status::Unknown);
}

void TestFfmpegProgressParser::testParseUnrelatedLine() {
    FfmpegProgressParser p;
    Info info = p.parseLine("frame=1234 fps=29.97 bitrate=ignored");
    // "bitrate=ignored" doesn't match the kbits/s regex, so all absent
    QVERIFY(!info.hasTime());
    QVERIFY(!info.hasSpeed());
    QVERIFY(!info.hasBitrate());
    QVERIFY(!info.hasSize());
    QCOMPARE(info.status, Info::Status::Unknown);
}

void TestFfmpegProgressParser::testParseTimeOnly() {
    FfmpegProgressParser p;
    Info info = p.parseLine("time=00:01:30.50");
    QVERIFY(info.hasTime());
    QCOMPARE(info.timeMs, qint64(90 * 1000 + 500));  // 90.5s -> 90500 ms
    QVERIFY(!info.hasSpeed());
    QVERIFY(!info.hasBitrate());
    QVERIFY(!info.hasSize());
    QCOMPARE(info.status, Info::Status::Unknown);
}

void TestFfmpegProgressParser::testParseSpeedOnly() {
    FfmpegProgressParser p;
    Info info = p.parseLine("speed=1.5x");
    QVERIFY(!info.hasTime());
    QVERIFY(info.hasSpeed());
    QCOMPARE(info.speed, 1.5);
    QVERIFY(!info.hasBitrate());
    QVERIFY(!info.hasSize());
}

void TestFfmpegProgressParser::testParseBitrateOnly() {
    FfmpegProgressParser p;
    Info info = p.parseLine("bitrate=1024.0kbits/s");
    QVERIFY(!info.hasTime());
    QVERIFY(!info.hasSpeed());
    QVERIFY(info.hasBitrate());
    QCOMPARE(info.bitrateKbps, 1024.0);
    QVERIFY(!info.hasSize());
}

void TestFfmpegProgressParser::testParseSizeOnly() {
    FfmpegProgressParser p;
    Info info = p.parseLine("size=512kB");
    QVERIFY(info.hasSize());
    QCOMPARE(info.sizeBytes, qint64(512 * 1024));
    QVERIFY(!info.hasTime());
}

void TestFfmpegProgressParser::testParseProgressContinue() {
    FfmpegProgressParser p;
    Info info = p.parseLine("progress=continue");
    QCOMPARE(info.status, Info::Status::Continue);
}

void TestFfmpegProgressParser::testParseProgressEnd() {
    FfmpegProgressParser p;
    Info info = p.parseLine("progress=end");
    QCOMPARE(info.status, Info::Status::End);
}

void TestFfmpegProgressParser::testParseProgressUnknown() {
    FfmpegProgressParser p;
    // A different status value should not match; status stays Unknown.
    Info info = p.parseLine("progress=garbage");
    QCOMPARE(info.status, Info::Status::Unknown);
}

void TestFfmpegProgressParser::testParseFullLine() {
    FfmpegProgressParser p;
    Info info = p.parseLine(
        "frame=120 fps=30.0 stream_0_0_q=28.0 bitrate=2048.5kbits/s "
        "total_size=2048kB time=00:00:10.05 speed=2.0x progress=continue");
    QVERIFY(info.hasTime());
    QCOMPARE(info.timeMs, qint64(10 * 1000 + 50));  // 10.05s -> 10050 ms
    QVERIFY(info.hasSpeed());
    QCOMPARE(info.speed, 2.0);
    QVERIFY(info.hasBitrate());
    QCOMPARE(info.bitrateKbps, 2048.5);
    QVERIFY(info.hasSize());
    QCOMPARE(info.sizeBytes, qint64(2048 * 1024));
    QCOMPARE(info.status, Info::Status::Continue);
}

void TestFfmpegProgressParser::testMalformedTime() {
    FfmpegProgressParser p;
    Info info = p.parseLine("time=abc");
    QVERIFY(!info.hasTime());
}

void TestFfmpegProgressParser::testMalformedSpeed() {
    FfmpegProgressParser p;
    Info info = p.parseLine("speed=fastx");
    QVERIFY(!info.hasSpeed());
}

void TestFfmpegProgressParser::testMalformedSize() {
    FfmpegProgressParser p;
    Info info = p.parseLine("size=12MB");  // MB suffix not supported by regex
    QVERIFY(!info.hasSize());
}

void TestFfmpegProgressParser::testParseTimeToMsHappy() {
    QCOMPARE(FfmpegProgressParser::parseTimeToMs("01:02:03.04"),
             qint64(3723 * 1000 + 40));  // 1*3600 + 2*60 + 3 = 3723 s, .04 -> 40 ms
}

void TestFfmpegProgressParser::testParseTimeToMsZero() {
    QCOMPARE(FfmpegProgressParser::parseTimeToMs("00:00:00.00"), qint64(0));
}

void TestFfmpegProgressParser::testParseTimeToMsMalformed() {
    QCOMPARE(FfmpegProgressParser::parseTimeToMs("not-a-time"), qint64(-1));
    QCOMPARE(FfmpegProgressParser::parseTimeToMs("1:2:3"), qint64(-1));        // not 2-digit groups
    QCOMPARE(FfmpegProgressParser::parseTimeToMs("aa:bb:cc.dd"), qint64(-1));
}

void TestFfmpegProgressParser::testParseTimeToMsEmpty() {
    QCOMPARE(FfmpegProgressParser::parseTimeToMs(""), qint64(-1));
}

void TestFfmpegProgressParser::testParserIsStateless() {
    FfmpegProgressParser p;
    const QString line = "time=00:00:05.00 speed=1.25x progress=continue";
    Info first = p.parseLine(line);
    Info second = p.parseLine(line);
    QCOMPARE(first.timeMs, second.timeMs);
    QCOMPARE(first.speed, second.speed);
    QCOMPARE(first.status, second.status);
}
