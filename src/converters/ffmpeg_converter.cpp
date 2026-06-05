#include "ffmpeg_converter.h"
#include "config_manager.h"
#include "logger.h"
#include "error_handler.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

static constexpr qint64 WAIT_FOR_FINISHED_TIMEOUT_MS = 300000; // 5 min

FFmpegConverter::FFmpegConverter(QObject* parent)
    : QObject(parent)
    , m_ffmpegPath("ffmpeg")
    , m_ffprobePath("ffprobe")
    , m_process(nullptr)
    , m_isRunning(false)
    , m_totalDuration(0.0)
    , m_currentSpeed(0.0)
    , m_estimatedRemainingMs(0)
    , m_currentBitrate(0.0)
    , m_processedBytes(0)
    , m_conversionStartTime(0)
{
    m_ffmpegPath = ConfigManager::instance().value("ffmpegPath", "ffmpeg").toString();
    m_ffprobePath = ConfigManager::instance().value("ffprobePath", "ffprobe").toString();
    const auto& reg = FormatRegistry::instance();
    m_videoFormats = reg.videoFormats();
    m_audioFormats = reg.audioFormats();
    m_timeRegex = QRegularExpression(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    m_progressRegex = QRegularExpression(R"(progress=(\w+))");
    m_speedRegex = QRegularExpression(R"(speed=\s*([\d.]+)x)");
    m_bitrateRegex = QRegularExpression(R"(bitrate=\s*([\d.]+)kbits/s)");
    m_sizeRegex = QRegularExpression(R"(size=\s*(\d+)kB)");
}

FFmpegConverter::~FFmpegConverter() {
    cancel();
}

bool FFmpegConverter::isVideoFormat(const QString& format) const {
    return m_videoFormats.contains(format.toLower());
}

bool FFmpegConverter::isAudioFormat(const QString& format) const {
    return m_audioFormats.contains(format.toLower());
}

QStringList FFmpegConverter::supportedInputFormats() const {
    return m_videoFormats + m_audioFormats;
}

QStringList FFmpegConverter::supportedOutputFormats() const {
    return m_videoFormats + m_audioFormats;
}

bool FFmpegConverter::isConversionSupported(const QString& inputFormat,
                                           const QString& outputFormat) const {
    QString input = inputFormat.toLower();
    QString output = outputFormat.toLower();
    bool inputSupported = m_videoFormats.contains(input) || m_audioFormats.contains(input);
    bool outputSupported = m_videoFormats.contains(output) || m_audioFormats.contains(output);
    return inputSupported && outputSupported;
}

QString FFmpegConverter::getFormatFromExtension(const QString& filePath) const {
    return QFileInfo(filePath).suffix().toLower();
}

int FFmpegConverter::parseTimeToMs(const QString& timeStr) {
    QRegularExpressionMatch match = m_timeRegex.match(timeStr);
    if (!match.hasMatch()) {
        return -1;
    }
    int hours = match.captured(1).toInt();
    int minutes = match.captured(2).toInt();
    int seconds = match.captured(3).toInt();
    int centiseconds = match.captured(4).toInt();
    return hours * 3600000 + minutes * 60000 + seconds * 1000 + centiseconds * 10;
}

void FFmpegConverter::updateEstimatedTime(int currentProgress) {
    if (currentProgress <= 0 || currentProgress >= 100) {
        m_estimatedRemainingMs = 0;
        return;
    }
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_progressHistory.append(qMakePair(currentProgress, now));
    while (m_progressHistory.size() > 10) {
        m_progressHistory.removeFirst();
    }
    if (m_progressHistory.size() >= 2) {
        const auto& first = m_progressHistory.first();
        const auto& last = m_progressHistory.last();
        int progressDiff = last.first - first.first;
        qint64 timeDiff = last.second - first.second;
        if (progressDiff > 0 && timeDiff > 0) {
            double speed = static_cast<double>(progressDiff) / static_cast<double>(timeDiff);
            int remainingProgress = 100 - currentProgress;
            m_estimatedRemainingMs = static_cast<qint64>(remainingProgress / speed);
        }
    }
}

void FFmpegConverter::parseProgress(const QString& line) {
    if (m_totalDuration <= 0) {
        return;
    }
    int timeMs = parseTimeToMs(line);
    int progress = 0;
    if (timeMs > 0) {
        double currentSeconds = timeMs / 1000.0;
        progress = static_cast<int>((currentSeconds / m_totalDuration) * 100.0);
        progress = qBound(0, progress, 100);
        emit progressChanged(progress);
    }
    QRegularExpressionMatch speedMatch = m_speedRegex.match(line);
    if (speedMatch.hasMatch()) {
        m_currentSpeed = speedMatch.captured(1).toDouble();
    }
    QRegularExpressionMatch bitrateMatch = m_bitrateRegex.match(line);
    if (bitrateMatch.hasMatch()) {
        m_currentBitrate = bitrateMatch.captured(1).toDouble();
    }
    QRegularExpressionMatch sizeMatch = m_sizeRegex.match(line);
    if (sizeMatch.hasMatch()) {
        m_processedBytes = sizeMatch.captured(1).toLongLong() * 1024;
    }
    if (m_currentSpeed > 0 && progress > 0 && progress < 100) {
        double remainingSeconds = (m_totalDuration * (100 - progress) / 100.0) / m_currentSpeed;
        m_estimatedRemainingMs = static_cast<qint64>(remainingSeconds * 1000);
    } else {
        updateEstimatedTime(progress);
    }
    emit detailedProgress(progress, m_currentSpeed, m_estimatedRemainingMs, m_currentBitrate);
    QRegularExpressionMatch progressMatch = m_progressRegex.match(line);
    if (progressMatch.hasMatch()) {
        QString status = progressMatch.captured(1);
        if (status == "continue") {
            emit statusChanged(tr("正在转换..."));
        } else if (status == "end") {
            emit statusChanged(tr("转换完成"));
        }
    }
}

// Pixel format selection — kept as an internal enum for the FFmpeg converter.
// Stored in QVariantMap as int: YUV420P=1, YUV422P=2, YUV444P=3.
enum class PixelFormat : int {
    Yuv420p = 1,
    Yuv422p = 2,
    Yuv444p = 3
};

QStringList FFmpegConverter::buildVideoArgs(const QVariantMap& params) {
    QStringList args;
    QString codec = params.value("videoCodec").toString();
    if (codec.isEmpty()) {
        codec = params.value("codec").toString();
    }
    if (!codec.isEmpty() && codec.toLower() != "auto") {
        const auto& reg = FormatRegistry::instance();
        QString ffmpegCodec = reg.ffmpegVideoCodec(codec);
        args << "-c:v" << ffmpegCodec;
        if (codec.toLower() == "h264" || codec.toLower() == "h265" || codec.toLower() == "hevc") {
            args << "-preset" << params.value("preset", "medium").toString();
        }
    }
    int bitrate = params.value("videoBitrate", 0).toInt();
    if (bitrate <= 0) {
        bitrate = params.value("bitrate", 0).toInt();
    }
    if (bitrate > 0) {
        args << "-b:v" << QString("%1k").arg(bitrate);
    }
    int crf = params.value("crf", -1).toInt();
    if (crf >= 0 && crf <= 51) {
        args << "-crf" << QString::number(crf);
    }
    QString resolution = params.value("resolution").toString();
    if (!resolution.isEmpty()) {
        args << "-s" << resolution;
    }
    int width = params.value("width", 0).toInt();
    int height = params.value("height", 0).toInt();
    if (width > 0 && height > 0) {
        args << "-s" << QString("%1x%2").arg(width).arg(height);
    }
    int fps = params.value("fps", 0).toInt();
    if (fps <= 0) {
        fps = params.value("frameRate", 0).toInt();
    }
    if (fps > 0) {
        args << "-r" << QString::number(fps);
    }
    QString aspectRatio = params.value("aspectRatio").toString();
    if (!aspectRatio.isEmpty()) {
        args << "-aspect" << aspectRatio;
    }
    int rawPf = params.value("pixelFormat", 0).toInt();
    auto pf = static_cast<PixelFormat>(rawPf);
    switch (pf) {
    case PixelFormat::Yuv420p: args << "-pix_fmt" << "yuv420p"; break;
    case PixelFormat::Yuv422p: args << "-pix_fmt" << "yuv422p"; break;
    case PixelFormat::Yuv444p: args << "-pix_fmt" << "yuv444p"; break;
    default: break;
    }
    return args;
}

QStringList FFmpegConverter::buildAudioArgs(const QVariantMap& params) {
    QStringList args;
    QString codec = params.value("audioCodec").toString();
    if (!codec.isEmpty()) {
        const auto& reg = FormatRegistry::instance();
        args << "-c:a" << reg.ffmpegAudioCodec(codec);
    }
    int bitrate = params.value("audioBitrate", 0).toInt();
    if (bitrate > 0) {
        args << "-b:a" << QString("%1k").arg(bitrate);
    }
    int sampleRate = params.value("sampleRate", 0).toInt();
    if (sampleRate > 0) {
        args << "-ar" << QString::number(sampleRate);
    }
    int channels = params.value("channels", 0).toInt();
    if (channels > 0) {
        args << "-ac" << QString::number(channels);
    }
    // VBR quality for MP3/Vorbis/Opus (0=best, 9=worst for MP3; -1=auto)
    // For AAC, map to -vbr flag instead
    int vbrQuality = params.value("vbrQuality", -1).toInt();
    if (vbrQuality >= 0 && vbrQuality <= 9) {
        QString acodec = params.value("audioCodec").toString().toLower();
        if (acodec == "mp3" || acodec == "libmp3lame") {
            args << "-qscale:a" << QString::number(vbrQuality);
        } else if (acodec == "vorbis" || acodec == "libvorbis") {
            args << "-qscale:a" << QString::number(vbrQuality);
        } else if (acodec == "opus" || acodec == "libopus") {
            // Opus: 0-10 in 0.5 increments, map 0-9 → 0-10
            double opusQuality = qBound(0.0, vbrQuality * 10.0 / 9.0, 10.0);
            args << "-qscale:a" << QString::number(opusQuality, 'f', 1);
        }
    }
    return args;
}

static const QStringList s_validVideoCodecs = {
    "libx264", "libx265", "libvpx-vp9", "mpeg4", "h264_nvenc", "h264", "h265", "hevc", "vp9", "av1", "auto"
};
static const QStringList s_validAudioCodecs = {
    "libmp3lame", "aac", "libvorbis", "vorbis", "flac", "pcm_s16le", "libopus", "mp3", "opus"
};
static const QStringList s_validPresets = {
    "ultrafast", "superfast", "veryfast", "faster", "fast",
    "medium", "slow", "slower", "veryslow"
};
static const QSet<int> s_validSampleRates = {8000, 11025, 16000, 22050, 44100, 48000, 96000};
static const QSet<int> s_validChannels = {1, 2};
static const QRegularExpression s_resolutionRe(R"(^\d+x\d+$)");

bool FFmpegConverter::validateParams(const QVariantMap& params, QString& errorMsg) {
    // Validate video codec
    QString videoCodec = params.value("videoCodec").toString();
    if (videoCodec.isEmpty()) {
        videoCodec = params.value("codec").toString();
    }
    if (!videoCodec.isEmpty() && !s_validVideoCodecs.contains(videoCodec.toLower())) {
        errorMsg = QString("不支持的视频编码器: %1").arg(videoCodec);
        return false;
    }

    // Validate audio codec
    QString audioCodec = params.value("audioCodec").toString();
    if (!audioCodec.isEmpty() && !s_validAudioCodecs.contains(audioCodec.toLower())) {
        errorMsg = QString("不支持的音频编码器: %1").arg(audioCodec);
        return false;
    }

    // Validate resolution format
    QString resolution = params.value("resolution").toString();
    if (!resolution.isEmpty() && !s_resolutionRe.match(resolution).hasMatch()) {
        errorMsg = QString("无效的分辨率格式: %1 (期望格式: 宽x高，如 1920x1080)").arg(resolution);
        return false;
    }

    // Validate video bitrate
    int videoBitrate = params.value("videoBitrate", 0).toInt();
    if (videoBitrate < 0) {
        errorMsg = QString("视频比特率不能为负数: %1").arg(videoBitrate);
        return false;
    }
    if (videoBitrate > 50000) {
        errorMsg = QString("视频比特率过高: %1 kbps (上限: 50000)").arg(videoBitrate);
        return false;
    }

    // Validate audio bitrate
    int audioBitrate = params.value("audioBitrate", 0).toInt();
    if (audioBitrate < 0) {
        errorMsg = QString("音频比特率不能为负数: %1").arg(audioBitrate);
        return false;
    }
    if (audioBitrate > 512) {
        errorMsg = QString("音频比特率过高: %1 kbps (上限: 512)").arg(audioBitrate);
        return false;
    }

    // Validate sample rate
    int sampleRate = params.value("sampleRate", 0).toInt();
    if (sampleRate != 0 && !s_validSampleRates.contains(sampleRate)) {
        errorMsg = QString("无效的采样率: %1 Hz (可选: 8000, 11025, 16000, 22050, 44100, 48000, 96000)").arg(sampleRate);
        return false;
    }

    // Validate channels
    int channels = params.value("channels", 0).toInt();
    if (channels != 0 && !s_validChannels.contains(channels)) {
        errorMsg = QString("无效的声道数: %1 (仅支持 1 或 2)").arg(channels);
        return false;
    }

    // Validate preset
    QString preset = params.value("preset").toString();
    if (!preset.isEmpty() && !s_validPresets.contains(preset.toLower())) {
        errorMsg = QString("无效的编码预设: %1").arg(preset);
        return false;
    }

    // Validate pixel format
    int pixelFormat = params.value("pixelFormat", 0).toInt();
    if (pixelFormat < 0 || pixelFormat > 3) {
        errorMsg = QString("无效的像素格式: %1").arg(pixelFormat);
        return false;
    }

    // Validate frame rate
    int fps = params.value("fps", 0).toInt();
    if (fps < 0) {
        errorMsg = QString("帧率不能为负数: %1").arg(fps);
        return false;
    }
    if (fps > 120) {
        errorMsg = QString("帧率过高: %1 (上限: 120)").arg(fps);
        return false;
    }

    return true;
}

double FFmpegConverter::getDuration(const QString& filePath) {
    QStringList args;
    args << "-v" << "error" << "-show_entries" << "format=duration"
         << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath;
    QString output;
    if (runFFprobe(args, output)) {
        return output.trimmed().toDouble();
    }
    return 0.0;
}

bool FFmpegConverter::getMediaInfo(const QString& filePath, QVariantMap& info) {
    QStringList args;
    args << "-v" << "quiet" << "-print_format" << "json"
         << "-show_format" << "-show_streams" << filePath;
    QString output;
    if (!runFFprobe(args, output)) {
        return false;
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR("FFmpeg", QString("JSON解析错误: %1").arg(parseError.errorString()));
        return false;
    }
    QJsonObject root = doc.object();
    QJsonObject format = root["format"].toObject();
    info["duration"] = format["duration"].toDouble();
    info["bitRate"] = format["bit_rate"].toString().toLongLong();
    info["formatName"] = format["format_name"].toString();
    QJsonArray streams = root["streams"].toArray();
    for (const QJsonValue& streamVal : streams) {
        QJsonObject stream = streamVal.toObject();
        QString codecType = stream["codec_type"].toString();
        if (codecType == "video" && !info.contains("videoWidth")) {
            info["videoWidth"] = stream["width"].toInt();
            info["videoHeight"] = stream["height"].toInt();
            info["videoCodec"] = stream["codec_name"].toString();
            info["fps"] = stream["r_frame_rate"].toString();
        } else if (codecType == "audio" && !info.contains("audioCodec")) {
            info["audioCodec"] = stream["codec_name"].toString();
            info["sampleRate"] = stream["sample_rate"].toInt();
            info["channels"] = stream["channels"].toInt();
        }
    }
    return true;
}

bool FFmpegConverter::extractAudio(const QString& inputFile, const QString& outputFile,
                                  const QVariantMap& params) {
    LOG_INFO("FFmpeg", QString("提取音频: %1 -> %2").arg(inputFile, outputFile));
    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists()) {
        LOG_ERROR("FFmpeg", QString("输入文件不存在: %1").arg(inputFile));
        emit conversionFinished(false, tr("输入文件不存在"));
        return false;
    }
    m_totalDuration = getDuration(inputFile);
    m_currentOutputFile = outputFile;
    m_errorBuffer.clear();
    m_conversionStartTime = QDateTime::currentMSecsSinceEpoch();
    m_progressHistory.clear();
    m_currentSpeed = 0.0;
    m_estimatedRemainingMs = 0;
    m_currentBitrate = 0.0;
    m_processedBytes = 0;
    QStringList args;
    args << "-y" << "-i" << inputFile << "-vn";
    args << buildAudioArgs(params);
    QString audioCodec = params.value("audioCodec").toString();
    if (audioCodec.isEmpty()) {
        QString outputExt = getFormatFromExtension(outputFile);
        if (outputExt == "mp3") {
            args << "-c:a" << "libmp3lame";
        } else if (outputExt == "aac") {
            args << "-c:a" << "aac";
        } else if (outputExt == "flac") {
            args << "-c:a" << "flac";
        } else if (outputExt == "ogg") {
            args << "-c:a" << "libopus";
        }
    }
    args << outputFile;
    return runFFmpeg(args);
}

bool FFmpegConverter::convert(const QString& inputFile, const QString& outputFile,
                             const QVariantMap& params) {
    LOG_INFO("FFmpeg", QString("开始转换: %1 -> %2").arg(inputFile, outputFile));
    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists()) {
        ErrorInfo error = ErrorTypes::createFileNotFoundError(inputFile, "FFmpeg::convert");
        error.outputFile = outputFile;
        m_lastError = error;
        LOG_ERROR("FFmpeg", QString("输入文件不存在: %1").arg(inputFile));
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return false;
    }
    // Validate parameters before proceeding
    QString paramError;
    if (!validateParams(params, paramError)) {
        ErrorInfo error = ErrorTypes::createError(
            ErrorCode::InvalidParameter, paramError, "FFmpeg::convert");
        error.inputFile = inputFile;
        error.outputFile = outputFile;
        m_lastError = error;
        LOG_ERROR("FFmpeg", paramError);
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, paramError);
        return false;
    }

    m_totalDuration = getDuration(inputFile);
    m_currentOutputFile = outputFile;
    m_currentInputFile = inputFile;
    m_errorBuffer.clear();
    m_conversionStartTime = QDateTime::currentMSecsSinceEpoch();
    m_progressHistory.clear();
    m_currentSpeed = 0.0;
    m_estimatedRemainingMs = 0;
    m_currentBitrate = 0.0;
    m_processedBytes = 0;
    QString inputFormat = getFormatFromExtension(inputFile);
    QString outputFormat = getFormatFromExtension(outputFile);
    bool inputIsVideo = isVideoFormat(inputFormat);
    bool outputIsVideo = isVideoFormat(outputFormat);
    bool inputIsAudio = isAudioFormat(inputFormat);
    bool outputIsAudio = isAudioFormat(outputFormat);
    QStringList args;
    args << "-y" << "-i" << inputFile;
    bool twoPass = params.value("twoPass", false).toBool() && inputIsVideo && outputIsVideo;

    if (inputIsVideo && outputIsVideo) {
        QStringList videoArgs = buildVideoArgs(params);
        QStringList audioArgs = buildAudioArgs(params);
        if (twoPass) {
            // Pass 1: video only, no audio, output to null
            QStringList pass1Args;
            pass1Args << "-y" << "-i" << inputFile
                      << videoArgs
                      << "-an" << "-pass" << "1"
                      << "-f" << "null";
#ifdef Q_OS_WIN
            pass1Args << "NUL";
#else
            pass1Args << "/dev/null";
#endif
            LOG_INFO("FFmpeg", "开始二遍编码第一遍 (分析)");
            if (!runFFmpeg(pass1Args)) {
                return false;
            }
            // Reset state for second pass (runFFmpeg resets m_isRunning etc.)
            m_errorBuffer.clear();
            m_conversionStartTime = QDateTime::currentMSecsSinceEpoch();
            m_progressHistory.clear();
            m_currentSpeed = 0.0;
            m_estimatedRemainingMs = 0;
            m_currentBitrate = 0.0;
            m_processedBytes = 0;
            m_totalDuration = getDuration(inputFile);
            // Pass 2: video + audio with actual output
            args.clear();
            args << "-y" << "-i" << inputFile
                 << videoArgs
                 << audioArgs
                 << "-pass" << "2"
                 << outputFile;
            LOG_INFO("FFmpeg", "开始二遍编码第二遍 (输出)");
            return runFFmpeg(args);
        }
        args << videoArgs;
        args << audioArgs;
    } else if (inputIsVideo && outputIsAudio) {
        args << "-vn";
        args << buildAudioArgs(params);
        QString audioCodec = params.value("audioCodec").toString();
        if (audioCodec.isEmpty()) {
            if (outputFormat == "mp3") {
                args << "-c:a" << "libmp3lame";
            } else if (outputFormat == "aac") {
                args << "-c:a" << "aac";
            } else if (outputFormat == "flac") {
                args << "-c:a" << "flac";
            }
        }
    } else if (inputIsAudio && outputIsAudio) {
        args << buildAudioArgs(params);
    } else if (inputIsAudio && outputIsVideo) {
        LOG_WARNING("FFmpeg", tr("从音频转换为视频需要提供额外的视频素材"));
        args << buildVideoArgs(params);
    }
    args << outputFile;
    return runFFmpeg(args);
}

bool FFmpegConverter::runFFmpeg(const QStringList& args) {
    if (m_isRunning) {
        LOG_ERROR("FFmpeg", tr("已有转换任务在运行"));
        return false;
    }
    // Re-create the process to ensure clean state and fresh connections.
    // Managed by unique_ptr — no Qt parent to avoid double-delete.
    m_process = std::make_unique<QProcess>();
    m_process->setProgram(m_ffmpegPath);
    m_process->setArguments(args);
    // MergedChannels ensures progress text lands in stderr (where ffmpeg writes
    // -progress data) but also makes stdout accessible if a future caller
    // needs it. This is the recommended mode for ffmpeg/ffprobe pipelines.
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process.get(), &QProcess::readyReadStandardError,
            this, &FFmpegConverter::onProcessReadyReadStandardError);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegConverter::onProcessFinished);
    connect(m_process.get(), &QProcess::errorOccurred,
            this, &FFmpegConverter::onProcessError);
    LOG_DEBUG("FFmpeg", QString("执行命令: %1 %2").arg(m_ffmpegPath, args.join(" ")));
    m_isRunning = true;
    emit statusChanged(tr("正在启动FFmpeg..."));
    m_process->start();
    if (!m_process->waitForStarted(5000)) {
        m_isRunning = false;
        ErrorInfo error = ErrorTypes::createProcessError(
            ErrorCode::ProcessFailedToStart, "FFmpeg",
            m_process->errorString(), "FFmpeg::runFFmpeg");
        error.inputFile = m_currentInputFile;
        error.outputFile = m_currentOutputFile;
        m_lastError = error;
        LOG_ERROR("FFmpeg", error.message);
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return false;
    }
    emit statusChanged(tr("正在转换..."));
    // Wait for the process with a timeout so a hung ffmpeg does not
    // permanently block the worker thread.
    if (!m_process->waitForFinished(WAIT_FOR_FINISHED_TIMEOUT_MS)) {
        LOG_WARNING("FFmpeg", tr("FFmpeg 进程超时，正在终止"));
        m_process->kill();
        m_process->waitForFinished(5000);
        m_isRunning = false;
        ErrorInfo error = ErrorTypes::createError(
            ErrorCode::TaskTimeout,
            tr("FFmpeg 转换超时"),
            "FFmpeg::runFFmpeg");
        error.inputFile = m_currentInputFile;
        error.outputFile = m_currentOutputFile;
        m_lastError = error;
        LOG_ERROR("FFmpeg", error.message);
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return false;
    }
    bool success = m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0;
    m_isRunning = false;
    // Destroy the finished QProcess to close OS pipe handles and prevent stale
    // queued signal events (readyReadStandardError, finished) from firing during
    // the nested event loop of QDialog::exec() in the completion flow.
    m_process.reset();
    return success;
}

bool FFmpegConverter::runFFprobe(const QStringList& args, QString& output) {
    QProcess process;
    process.setProgram(m_ffprobePath);
    process.setArguments(args);
    LOG_DEBUG("FFmpeg", QString("执行FFprobe: %1 %2").arg(m_ffprobePath, args.join(" ")));
    process.start();
    if (!process.waitForStarted(5000)) {
        output = tr("无法启动FFprobe进程");
        return false;
    }
    if (!process.waitForFinished(30000)) {
        output = tr("FFprobe进程执行超时");
        return false;
    }
    output = QString::fromUtf8(process.readAllStandardOutput());
    return process.exitCode() == 0;
}

void FFmpegConverter::onProcessReadyReadStandardError() {
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardError());
    m_errorBuffer += output;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        parseProgress(line);
    }
}

void FFmpegConverter::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    // Guard: if already marked not running (e.g. runFFmpeg already reset after
    // waitForFinished), skip to avoid double-emission of completion signals.
    if (!m_isRunning) return;
    m_isRunning = false;
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    if (success) {
        LOG_INFO("FFmpeg", QString("转换完成: %1").arg(m_currentOutputFile));
        emit progressChanged(100);
        emit statusChanged(tr("转换完成"));
        emit conversionFinished(true, tr("转换成功"));
    } else {
        ErrorInfo error;
        if (exitStatus == QProcess::CrashExit) {
            error = ErrorTypes::createProcessError(ErrorCode::ProcessCrashed, "FFmpeg",
                                                   QString(), "FFmpeg::onProcessFinished");
        } else {
            QString detailMsg;
            if (!m_errorBuffer.isEmpty()) {
                QStringList lines = m_errorBuffer.split('\n', Qt::SkipEmptyParts);
                for (int i = lines.size() - 1; i >= 0 && detailMsg.length() < 500; --i) {
                    if (lines[i].contains("Error") || lines[i].contains("error") ||
                        lines[i].contains("Invalid") || lines[i].contains("failed")) {
                        detailMsg += lines[i].trimmed() + "\n";
                    }
                }
            }
            error = ErrorTypes::createConversionFailedError(
                detailMsg.trimmed().isEmpty()
                    ? tr("退出码: %1").arg(exitCode)
                    : detailMsg.trimmed(),
                "FFmpeg", "FFmpeg::onProcessFinished");
        }
        error.inputFile = m_currentInputFile;
        error.outputFile = m_currentOutputFile;
        m_lastError = error;
        LOG_ERROR("FFmpeg", error.fullMessage());
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit statusChanged(tr("转换失败"));
        emit conversionFinished(false, error.message);
    }
}

void FFmpegConverter::onProcessError(QProcess::ProcessError error) {
    // Guard: if already handled (e.g. runFFmpeg already reset after waitForFinished),
    // skip to avoid double-emission of completion signals.
    if (!m_isRunning) return;
    m_isRunning = false;
    ErrorCode errorCode;
    switch (error) {
        case QProcess::FailedToStart:
            errorCode = ErrorCode::ProcessFailedToStart;
            break;
        case QProcess::Crashed:
            errorCode = ErrorCode::ProcessCrashed;
            break;
        case QProcess::Timedout:
            errorCode = ErrorCode::TaskTimeout;
            break;
        default:
            errorCode = ErrorCode::Unknown;
            break;
    }
    ErrorInfo err = ErrorTypes::createProcessError(errorCode, "FFmpeg",
                                                   QString(), "FFmpeg::onProcessError");
    err.inputFile = m_currentInputFile;
    err.outputFile = m_currentOutputFile;
    m_lastError = err;
    LOG_ERROR("FFmpeg", err.message);
    ErrorHandler::instance()->handleError(err);
    emit errorOccurred(err);
    emit statusChanged(tr("转换失败"));
    emit conversionFinished(false, err.message);
}

void FFmpegConverter::cancel() {
    if (m_isRunning && m_process) {
        LOG_INFO("FFmpeg", tr("取消转换任务"));
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process.reset();
        m_isRunning = false;
        emit statusChanged(tr("已取消"));
        emit conversionFinished(false, tr("用户取消"));
    }
}
