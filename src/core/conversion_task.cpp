#include "conversion_task.h"
#include <utility>

ConversionTask::ConversionTask(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_status(std::to_underlying(Status::Pending))
    , m_progress(0)
    , m_converterType(ConverterType::Unknown)
    , m_priority(Priority::Normal)
    , m_cancelled(0)
    , m_fileSize(0)
{
}

ConversionTask::ConversionTask(const QString& inputFile, const QString& outputFile,
                               const QVariantMap& params, QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_inputFile(inputFile)
    , m_outputFile(outputFile)
    , m_params(params)
    , m_status(std::to_underlying(Status::Pending))
    , m_progress(0)
    , m_converterType(ConverterType::Unknown)
    , m_priority(Priority::Normal)
    , m_cancelled(0)
    , m_fileSize(0)
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
        m_status.storeRelaxed(std::to_underlying(status));
        if (status == Status::Running) {
            m_startTime = QDateTime::currentDateTime();
            m_endTime = QDateTime();
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
        case ConverterType::FFmpeg:      return tr("FFmpeg");
        case ConverterType::Pandoc:      return tr("Pandoc");
        case ConverterType::ImageMagick: return tr("ImageMagick");
        case ConverterType::Unknown:     return tr("未知");
        default:                         return tr("未知");
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
    } else if (lower == "imagemagick") {
        return ConverterType::ImageMagick;
    }
    return ConverterType::Unknown;
}
