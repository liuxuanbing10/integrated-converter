#ifndef TEST_FFMPEG_PROGRESS_PARSER_H
#define TEST_FFMPEG_PROGRESS_PARSER_H

#include <QObject>

class TestFfmpegProgressParser : public QObject {
    Q_OBJECT
public:
    explicit TestFfmpegProgressParser(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    // Sentinel defaults
    void testDefaultStructIsAllAbsent();

    // Empty / blank / no-keys input
    void testParseEmptyLine();
    void testParseBlankLine();
    void testParseUnrelatedLine();

    // Individual keys
    void testParseTimeOnly();
    void testParseSpeedOnly();
    void testParseBitrateOnly();
    void testParseSizeOnly();
    void testParseProgressContinue();
    void testParseProgressEnd();
    void testParseProgressUnknown();

    // Full line
    void testParseFullLine();

    // Malformed input — fields should remain absent, no crash
    void testMalformedTime();
    void testMalformedSpeed();
    void testMalformedSize();

    // Static time converter
    void testParseTimeToMsHappy();
    void testParseTimeToMsZero();
    void testParseTimeToMsMalformed();
    void testParseTimeToMsEmpty();

    // Stateless: same input -> same output across calls
    void testParserIsStateless();
};
#endif
