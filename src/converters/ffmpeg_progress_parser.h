#ifndef FFMPEG_PROGRESS_PARSER_H
#define FFMPEG_PROGRESS_PARSER_H

#include <QString>

// Parsed snapshot of a single ffmpeg `-progress pipe:2` line.
// All fields default-construct to "not present" — callers should treat
// `timeMs < 0` as "no time data on this line" rather than "time = 0".
struct FfmpegProgressInfo {
    enum class Status {
        Unknown,    // line did not contain a `progress=` key
        Continue,   // `progress=continue`
        End         // `progress=end`
    };

    qint64 timeMs     = -1;     // -1 = no time on this line
    double speed      = -1.0;   // multiplier (e.g. 1.5x), -1 = not present
    double bitrateKbps = -1.0;  // kbits/s, -1 = not present
    qint64 sizeBytes  = -1;     // bytes, -1 = not present
    Status status     = Status::Unknown;

    bool hasTime()     const { return timeMs   >= 0; }
    bool hasSpeed()    const { return speed    >= 0; }
    bool hasBitrate()  const { return bitrateKbps >= 0; }
    bool hasSize()     const { return sizeBytes >= 0; }
};

// Stateless parser for ffmpeg's `-progress` machine-readable output.
//
// ffmpeg emits one `key=value` token per stderr write, e.g.
//
//     frame=1234
//     fps=29.97
//     ...
//     out_time_ms=12345678
//     progress=continue
//     ...
//     progress=end
//
// but the legacy `-progress pipe:1` (and the GUI's parser) also handle
// the older `time=HH:MM:SS.cc` format. This class accepts both.
class FfmpegProgressParser {
public:
    FfmpegProgressParser() = default;

    // Parse one line of ffmpeg progress output. Returns the extracted
    // values; fields not present in the line are left at their "absent"
    // sentinel. Blank lines and lines with no recognised keys return an
    // all-absent struct — they are not errors.
    FfmpegProgressInfo parseLine(const QString& line) const;

    // Convert "HH:MM:SS.cc" (centiseconds) to milliseconds. Returns -1 on
    // malformed input. Static so callers can also use it directly.
    static qint64 parseTimeToMs(const QString& timeStr);
};

#endif // FFMPEG_PROGRESS_PARSER_H
