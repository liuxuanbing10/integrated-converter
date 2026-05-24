#include "conversion_task.h"

ConversionTask::ConversionTask(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_status(static_cast<int>(Status::Pending))
    , m_progress(0)
    , m_converterType(ConverterType::Unknown)
    , m_priority(Priority::Normal)
    , m_cancelled(0)
    , m_fileSize(0)
    , m_estimatedRemainingMs(0)
    , m_processingSpeed(0.0)
    , m_currentBitrate(0.0)
{
}

ConversionTask::ConversionTask(const QString& inputFile, const QString& outputFile,
                               const QVariantMap& params, QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_inputFile(inputFile)
    , m_outputFile(outputFile)
    , m_params(params)
    , m_status(static_cast<int>(Status::Pending))
    , m_progress(0)
    , m_converterType(ConverterType::Unknown)
    , m_priority(Priority::Normal)
    , m_cancelled(0)
    , m_fileSize(0)
    , m_estimatedRemainingMs(0)
    , m_processingSpeed(0.0)
    , m_currentBitrate(0.0)
{
}

qint64 ConversionTask::durationMs() const {
    if (!m_startTime.isValid()) {
        return 0;
    }
    QDateTime end = m_endTime.isValid() ? m_endTime : QDateTime::currentDateTime();
    return m_startTime.msecsTo(end);
}

void ConversionTask::setStatus(Status status) {
    Status oldStatus = static_cast<Status>(m_status.loadRelaxed());
    if (oldStatus != status) {
        m_status.storeRelaxed(static_cast<int>(status));
        if (status == Status::Running) {
            m_startTime = QDateTime::currentDateTime();
            m_endTime = QDateTime();
            m_progressHistory.clear();
        } else if (status == Status::Completed || status == Status::Failed || status == Status::Cancelled) {
            m_endTime = QDateTime::currentDateTime();
        }
        emit statusChanged(status);
        if (status == Status::Completed) {
            emit finished(true, QString());
        } else if (status == Status::Failed) {
            emit finished(false, m_errorMessage);
        } else if (status == Status::Cancelled) {
            emit finished(false, tr("任务已取消"));
        }
    }
}

void ConversionTask::setProgress(int progress) {
    int clampedProgress = qBound(0, progress, 100);
    if (m_progress.loadRelaxed() != clampedProgress) {
        m_progress.storeRelaxed(clampedProgress);
        emit progressChanged(clampedProgress);
    }
}

void ConversionTask::updateEstimatedTime(int currentProgress, qint64 elapsedMs) {
    if (currentProgress <= 0 || currentProgress >= 100 || elapsedMs <= 0) {
        m_estimatedRemainingMs = 0;
        emit estimatedTimeChanged(0);
        return;
    }
    m_progressHistory.append(qMakePair(currentProgress, elapsedMs));
    while (m_progressHistory.size() > HISTORY_SIZE) {
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
            if (m_fileSize > 0 && elapsedMs > 0) {
                double processedRatio = currentProgress / 100.0;
                qint64 processedBytes = static_cast<qint64>(m_fileSize * processedRatio);
                m_processingSpeed = static_cast<double>(processedBytes) / (static_cast<double>(elapsedMs) / 1000.0);
            }
            emit estimatedTimeChanged(m_estimatedRemainingMs);
        }
    }
}

QString ConversionTask::statusToString(Status status) {
    switch (status) {
        case Status::Pending:   return tr("等待中");
        case Status::Running:   return tr("运行中");
        case Status::Completed: return tr("已完成");
        case Status::Failed:    return tr("失败");
        case Status::Cancelled: return tr("已取消");
        default:                return tr("未知");
    }
}

QString ConversionTask::converterTypeToString(ConverterType type) {
    switch (type) {
        case ConverterType::FFmpeg:  return tr("FFmpeg");
        case ConverterType::Pandoc:  return tr("Pandoc");
        case ConverterType::Unknown: return tr("未知");
        default:                     return tr("未知");
    }
}

QString ConversionTask::priorityToString(Priority priority) {
    switch (priority) {
        case Priority::Low:    return tr("低");
        case Priority::Normal: return tr("普通");
        case Priority::High:   return tr("高");
        default:               return tr("普通");
    }
}

ConversionTask::ConverterType ConversionTask::stringToConverterType(const QString& str) {
    QString lower = str.toLower();
    if (lower == "ffmpeg") {
        return ConverterType::FFmpeg;
    } else if (lower == "pandoc") {
        return ConverterType::Pandoc;
    }
    return ConverterType::Unknown;
}
