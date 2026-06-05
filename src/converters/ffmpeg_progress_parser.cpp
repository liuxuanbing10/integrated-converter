#include "ffmpeg_progress_parser.h"

#include <QRegularExpression>

namespace {
// Anchored to end-of-line so we don't false-match `time=` inside a longer
// token, and so consecutive writes like "time=00:00:01.50time=00:00:02.50"
// (which can happen when ffmpeg flushes a chunk) only pick up the first one.
const QRegularExpression& timeRegex() {
    static const QRegularExpression re(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    return re;
}
// Pure HH:MM:SS.cc pattern, used by the static parseTimeToMs().
const QRegularExpression& timeOnlyRegex() {
    static const QRegularExpression re(R"(^(\d{2}):(\d{2}):(\d{2})\.(\d{2})$)");
    return re;
}
const QRegularExpression& speedRegex() {
    static const QRegularExpression re(R"(speed=\s*([\d.]+)x)");
    return re;
}
const QRegularExpression& bitrateRegex() {
    static const QRegularExpression re(R"(bitrate=\s*([\d.]+)kbits/s)");
    return re;
}
const QRegularExpression& sizeRegex() {
    static const QRegularExpression re(R"(size=\s*(\d+)kB)");
    return re;
}
const QRegularExpression& progressRegex() {
    static const QRegularExpression re(R"(progress=(\w+))");
    return re;
}
} // namespace

qint64 FfmpegProgressParser::parseTimeToMs(const QString& timeStr) {
    // The static helper takes a PURE "HH:MM:SS.cc" string, not a
    // `key=value` line. We use a separate anchored regex so the bare
    // string is matched without the `time=` prefix.
    QRegularExpressionMatch match = timeOnlyRegex().match(timeStr);
    if (!match.hasMatch()) {
        return -1;
    }
    bool ok1, ok2, ok3, ok4;
    int hours       = match.captured(1).toInt(&ok1);
    int minutes     = match.captured(2).toInt(&ok2);
    int seconds     = match.captured(3).toInt(&ok3);
    int centisec    = match.captured(4).toInt(&ok4);
    if (!ok1 || !ok2 || !ok3 || !ok4) {
        return -1;
    }
    return static_cast<qint64>(hours) * 3600000
         + static_cast<qint64>(minutes) * 60000
         + static_cast<qint64>(seconds) * 1000
         + static_cast<qint64>(centisec) * 10;
}

FfmpegProgressInfo FfmpegProgressParser::parseLine(const QString& line) const {
    FfmpegProgressInfo out;

    if (auto m = timeRegex().match(line); m.hasMatch()) {
        // Extract the captured HH:MM:SS.cc substring and run it through
        // the pure-time parser so the conversion logic lives in one place.
        out.timeMs = parseTimeToMs(m.captured(0).mid(5));  // skip "time="
    }
    if (auto m = speedRegex().match(line); m.hasMatch()) {
        bool ok;
        double v = m.captured(1).toDouble(&ok);
        if (ok) out.speed = v;
    }
    if (auto m = bitrateRegex().match(line); m.hasMatch()) {
        bool ok;
        double v = m.captured(1).toDouble(&ok);
        if (ok) out.bitrateKbps = v;
    }
    if (auto m = sizeRegex().match(line); m.hasMatch()) {
        bool ok;
        qint64 v = m.captured(1).toLongLong(&ok);
        if (ok) out.sizeBytes = v * 1024;
    }
    if (auto m = progressRegex().match(line); m.hasMatch()) {
        const QString status = m.captured(1);
        if (status == QLatin1String("continue")) {
            out.status = FfmpegProgressInfo::Status::Continue;
        } else if (status == QLatin1String("end")) {
            out.status = FfmpegProgressInfo::Status::End;
        }
    }
    return out;
}
