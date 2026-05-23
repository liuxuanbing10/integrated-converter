#include "ffmpeg_converter.h"
#include "config_manager.h"
#include "logger.h"
#include "error_handler.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

FFmpegConverter::FFmpegConverter(QObject* parent)
    : QObject(parent)
    , m_ffmpegPath("ffmpeg")
    , m_ffprobePath("ffprobe")
    , m_process(nullptr)
    , m_timeoutTimer(nullptr)
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
    m_videoFormats << "mp4" << "avi" << "mkv" << "mov" << "wmv" << "flv" << "webm" << "mpeg";
    m_audioFormats << "mp3" << "wav" << "flac" << "aac" << "ogg" << "m4a" << "wma";
    m_timeRegex = QRegularExpression(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    m_progressRegex = QRegularExpression(R"(progress=(\w+))");
    m_speedRegex = QRegularExpression(R"(speed=\s*([\d.]+)x)");
    m_bitrateRegex = QRegularExpression(R"(bitrate=\s*([\d.]+)kbits/s)");
    m_sizeRegex = QRegularExpression(R"(size=\s*(\d+)kB)");
}

FFmpegConverter::~FFmpegConverter() {
    cancel();
}

QMap<QString, QString> FFmpegConverter::videoFormatMap() {
    static const QMap<QString, QString> formats = {
        {"mp4", "mp4"},
        {"avi", "avi"},
        {"flv", "flv"},
        {"mkv", "matroska"},
        {"webm", "webm"},
        {"mov", "mov"},
        {"wmv", "asf"},
        {"mpeg", "mpeg"}
    };
    return formats;
}

QMap<QString, QString> FFmpegConverter::audioFormatMap() {
    static const QMap<QString, QString> formats = {
        {"mp3", "mp3"},
        {"wav", "wav"},
        {"aac", "adts"},
        {"flac", "flac"},
        {"ogg", "ogg"},
        {"m4a", "mp4"}
    };
    return formats;
}

QMap<QString, QString> FFmpegConverter::videoCodecMap() {
    static const QMap<QString, QString> codecs = {
        {"h264", "libx264"},
        {"h265", "libx265"},
        {"hevc", "libx265"},
        {"vp9", "libvpx-vp9"},
        {"vp8", "libvpx"},
        {"av1", "libaom-av1"},
        {"mpeg4", "mpeg4"},
        {"mpeg2", "mpeg2video"}
    };
    return codecs;
}

QMap<QString, QString> FFmpegConverter::audioCodecMap() {
    static const QMap<QString, QString> codecs = {
        {"aac", "aac"},
        {"mp3", "libmp3lame"},
        {"opus", "libopus"},
        {"vorbis", "libvorbis"},
        {"flac", "flac"},
        {"pcm", "pcm_s16le"}
    };
    return codecs;
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

QStringList FFmpegConverter::buildVideoArgs(const QVariantMap& params) {
    QStringList args;
    QString codec = params.value("videoCodec").toString();
    if (codec.isEmpty()) {
        codec = params.value("codec").toString();
    }
    if (!codec.isEmpty()) {
        QMap<QString, QString> codecMap = videoCodecMap();
        QString ffmpegCodec = codecMap.value(codec.toLower(), codec);
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
    int pixelFormat = params.value("pixelFormat", 0).toInt();
    if (pixelFormat == 1) {
        args << "-pix_fmt" << "yuv420p";
    } else if (pixelFormat == 2) {
        args << "-pix_fmt" << "yuv422p";
    } else if (pixelFormat == 3) {
        args << "-pix_fmt" << "yuv444p";
    }
    return args;
}

QStringList FFmpegConverter::buildAudioArgs(const QVariantMap& params) {
    QStringList args;
    QString codec = params.value("audioCodec").toString();
    if (!codec.isEmpty()) {
        QMap<QString, QString> codecMap = audioCodecMap();
        QString ffmpegCodec = codecMap.value(codec.toLower(), codec);
        args << "-c:a" << ffmpegCodec;
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
    return args;
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
    if (inputIsVideo && outputIsVideo) {
        args << buildVideoArgs(params);
        args << buildAudioArgs(params);
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
    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setProgram(m_ffmpegPath);
    m_process->setArguments(args);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &FFmpegConverter::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegConverter::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
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
    return true;
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
        m_isRunning = false;
        emit statusChanged(tr("已取消"));
        emit conversionFinished(false, tr("用户取消"));
    }
}
