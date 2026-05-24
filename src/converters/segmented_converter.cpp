#include "segmented_converter.h"
#include "config_manager.h"
#include "logger.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

SegmentedConverter::SegmentedConverter(QObject* parent)
    : QObject(parent)
    , m_ffmpegPath("ffmpeg")
    , m_ffprobePath("ffprobe")
    , m_process(nullptr)
    , m_timeoutTimer(nullptr)
    , m_isRunning(false)
    , m_cancelled(false)
    , m_totalDuration(0.0)
    , m_totalSegments(4)
    , m_currentSegment(0)
    , m_segmentDuration(0.0)
    , m_currentSegmentProgress(0)
{
    m_ffmpegPath = ConfigManager::instance().value("ffmpegPath", "ffmpeg").toString();
    m_ffprobePath = ConfigManager::instance().value("ffprobePath", "ffprobe").toString();
}

SegmentedConverter::~SegmentedConverter() {
    cancel();
    cleanupTempFiles();
}

void SegmentedConverter::setSegmentSize(qint64 bytes) {
    Q_UNUSED(bytes);
}

void SegmentedConverter::setSegmentCount(int count) {
    m_totalSegments = qBound(2, count, 20);
}

void SegmentedConverter::setMaxSegments(int max) {
    m_totalSegments = qBound(2, max, 20);
}

int SegmentedConverter::overallProgress() const {
    if (m_totalSegments == 0) return 0;
    int baseProgress = (m_currentSegment * 100) / m_totalSegments;
    int segmentContrib = m_currentSegmentProgress / m_totalSegments;
    return qMin(99, baseProgress + segmentContrib);
}

bool SegmentedConverter::getDuration(const QString& filePath, double& duration) {
    QProcess process;
    process.setProgram(m_ffprobePath);
    QStringList args;
    args << "-v" << "error" << "-show_entries" << "format=duration"
         << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath;
    process.setArguments(args);
    process.start();
    if (!process.waitForStarted(5000) || !process.waitForFinished(30000)) {
        return false;
    }
    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    duration = output.toDouble();
    return duration > 0;
}

QString SegmentedConverter::formatTime(double seconds) const {
    int totalSec = static_cast<int>(seconds);
    int hours = totalSec / 3600;
    int minutes = (totalSec % 3600) / 60;
    int secs = totalSec % 60;
    int ms = static_cast<int>((seconds - totalSec) * 100);
    return QString("%1:%2:%3.%4")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(secs, 2, 10, QChar('0'))
           .arg(ms, 2, 10, QChar('0'));
}

bool SegmentedConverter::convert(const QString& input, const QString& output, const QVariantMap& params) {
    LOG_INFO("SegmentedConverter", QString("开始分段转换: %1 -> %2").arg(input, output));
    QFileInfo inputInfo(input);
    if (!inputInfo.exists()) {
        emit errorOccurred(tr("输入文件不存在"));
        emit conversionFinished(false, tr("输入文件不存在"));
        return false;
    }
    if (!getDuration(input, m_totalDuration)) {
        LOG_ERROR("SegmentedConverter", "无法获取视频时长");
        emit errorOccurred(tr("无法获取视频时长"));
        emit conversionFinished(false, tr("无法获取视频时长"));
        return false;
    }
    m_inputFile = input;
    m_outputFile = output;
    m_params = params;
    m_currentSegment = 0;
    m_currentSegmentProgress = 0;
    m_cancelled = false;
    m_segmentFiles.clear();
    m_tempDir = QDir::tempPath() + "/converter_segments_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(m_tempDir);
    m_segmentDuration = m_totalDuration / m_totalSegments;
    m_isRunning = true;
    emit statusChanged(tr("准备分段转换..."));
    return startNextSegment();
}

bool SegmentedConverter::startNextSegment() {
    if (m_cancelled) {
        cleanupTempFiles();
        emit conversionFinished(false, tr("用户取消"));
        return false;
    }
    if (m_currentSegment >= m_totalSegments) {
        emit statusChanged(tr("正在合并分段..."));
        return mergeSegments();
    }
    double startTime = m_currentSegment * m_segmentDuration;
    double endTime = (m_currentSegment + 1) * m_segmentDuration;
    bool isLastSegment = (m_currentSegment == m_totalSegments - 1);
    QString segmentFile = QString("%1/segment_%2.ts").arg(m_tempDir).arg(m_currentSegment, 3, 10, QChar('0'));
    m_segmentFiles.append(segmentFile);
    m_process = std::make_unique<QProcess>();
    QStringList args;
    args << "-y" << "-ss" << formatTime(startTime);
    if (!isLastSegment) {
        args << "-to" << formatTime(endTime - startTime);
    }
    args << "-i" << m_inputFile << "-c:v" << "libx264" << "-preset" << "fast"
         << "-c:a" << "aac" << "-f" << "mpegts" << segmentFile;
    m_process->setProgram(m_ffmpegPath);
    m_process->setArguments(args);
    connect(m_process.get(), &QProcess::readyReadStandardError, this, &SegmentedConverter::onSegmentReadyRead);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SegmentedConverter::onSegmentFinished);
    LOG_DEBUG("SegmentedConverter", QString("开始转换分段 %1/%2").arg(m_currentSegment + 1).arg(m_totalSegments));
    emit statusChanged(tr("转换分段 %1/%2...").arg(m_currentSegment + 1).arg(m_totalSegments));
    m_process->start();
    return m_process->waitForStarted(5000);
}

void SegmentedConverter::onSegmentReadyRead() {
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardError());
    QRegularExpression timeRegex(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch match = timeRegex.match(output);
    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        int seconds = match.captured(3).toInt();
        int centiseconds = match.captured(4).toInt();
        double currentTime = hours * 3600 + minutes * 60 + seconds + centiseconds / 100.0;
        if (m_segmentDuration > 0) {
            m_currentSegmentProgress = static_cast<int>((currentTime / m_segmentDuration) * 100);
            m_currentSegmentProgress = qBound(0, m_currentSegmentProgress, 100);
        }
        emit segmentProgress(m_currentSegment + 1, m_totalSegments, m_currentSegmentProgress);
        emit progressChanged(overallProgress());
    }
}

void SegmentedConverter::onSegmentFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        m_isRunning = false;
        cleanupTempFiles();
        QString error = tr("分段 %1 转换失败").arg(m_currentSegment + 1);
        LOG_ERROR("SegmentedConverter", error);
        emit errorOccurred(error);
        emit conversionFinished(false, error);
        return;
    }
    LOG_INFO("SegmentedConverter", QString("分段 %1/%2 完成").arg(m_currentSegment + 1).arg(m_totalSegments));
    m_currentSegment++;
    m_currentSegmentProgress = 100;
    emit segmentProgress(m_currentSegment, m_totalSegments, 100);
    emit progressChanged(overallProgress());
    startNextSegment();
}

bool SegmentedConverter::mergeSegments() {
    if (m_segmentFiles.isEmpty()) {
        m_isRunning = false;
        emit conversionFinished(false, tr("没有分段需要合并"));
        return false;
    }
    m_process = std::make_unique<QProcess>();
    QStringList args;
    args << "-y" << "-i" << "concat:" + m_segmentFiles.join("|");
    QString outputFormat = QFileInfo(m_outputFile).suffix().toLower();
    args << "-c" << "copy" << m_outputFile;
    m_process->setProgram(m_ffmpegPath);
    m_process->setArguments(args);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SegmentedConverter::onMergeFinished);
    LOG_DEBUG("SegmentedConverter", "开始合并分段");
    m_process->start();
    return m_process->waitForStarted(5000);
}

void SegmentedConverter::onMergeFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_isRunning = false;
    cleanupTempFiles();
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        LOG_INFO("SegmentedConverter", QString("分段转换完成: %1").arg(m_outputFile));
        emit progressChanged(100);
        emit statusChanged(tr("转换完成"));
        emit conversionFinished(true, tr("转换成功"));
    } else {
        QString error = tr("合并分段失败");
        LOG_ERROR("SegmentedConverter", error);
        emit errorOccurred(error);
        emit conversionFinished(false, error);
    }
}

void SegmentedConverter::cleanupTempFiles() {
    if (!m_tempDir.isEmpty()) {
        QDir dir(m_tempDir);
        if (dir.exists()) {
            dir.removeRecursively();
        }
        m_tempDir.clear();
    }
    m_segmentFiles.clear();
}

void SegmentedConverter::cancel() {
    if (m_isRunning) {
        m_cancelled = true;
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process.reset();
    }
        m_isRunning = false;
        cleanupTempFiles();
        emit statusChanged(tr("已取消"));
        emit conversionFinished(false, tr("用户取消"));
    }
}
