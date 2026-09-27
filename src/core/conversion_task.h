#ifndef CONVERSION_TASK_H
#define CONVERSION_TASK_H

#include <QAtomicInt>
#include <QAtomicInteger>
#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVariantMap>

class ConversionTask : public QObject
{
    Q_OBJECT

public:
    enum class Status
    {
        Pending,
        Running,
        Completed,
        Failed,
        Cancelled
    };

    enum class ConverterType
    {
        Unknown,
        FFmpeg,
        Pandoc,
        ImageMagick
    };

    enum class Priority
    {
        Low = 0,
        Normal = 1,
        High = 2
    };

    explicit ConversionTask(QObject* parent = nullptr);
    ConversionTask(const QString& inputFile, const QString& outputFile, const QVariantMap& params,
                   QObject* parent = nullptr);
    ~ConversionTask() override = default;

    QString id() const
    {
        return m_id;
    }
    QString inputFile() const
    {
        return m_inputFile;
    }
    QString outputFile() const
    {
        return m_outputFile;
    }
    QVariantMap params() const
    {
        return m_params;
    }
    Status status() const
    {
        return static_cast<Status>(m_status.loadRelaxed());
    }
    int progress() const
    {
        return m_progress.loadRelaxed();
    }
    QString errorMessage() const
    {
        QMutexLocker l(&m_dataMutex);
        return m_errorMessage;
    }
    ConverterType converterType() const
    {
        return m_converterType;
    }
    Priority priority() const
    {
        return m_priority;
    }
    QDateTime startTime() const
    {
        QMutexLocker l(&m_dataMutex);
        return m_startTime;
    }
    QDateTime endTime() const
    {
        QMutexLocker l(&m_dataMutex);
        return m_endTime;
    }
    qint64 durationMs() const;
    bool isCancelled() const
    {
        return m_cancelled.loadRelaxed() != 0;
    }
    /// Raw pointer to the atomic cancel flag — injected into the converter
    /// clone so its worker-thread wait-loop can poll cancellation.
    const QAtomicInt* cancelFlag() const
    {
        return &m_cancelled;
    }
    qint64 fileSize() const
    {
        return m_fileSize;
    }

    void setInputFile(const QString& file)
    {
        m_inputFile = file;
    }
    void setOutputFile(const QString& file)
    {
        m_outputFile = file;
    }
    void setParams(const QVariantMap& params)
    {
        m_params = params;
    }
    void setConverterType(ConverterType type)
    {
        m_converterType = type;
    }
    void setPriority(Priority priority)
    {
        m_priority = priority;
    }
    void setStatus(Status status);
    void setProgress(int progress);
    void setErrorMessage(const QString& message)
    {
        QMutexLocker l(&m_dataMutex);
        m_errorMessage = message;
    }
    void requestCancel()
    {
        m_cancelled.storeRelease(1);
    }
    void setFileSize(qint64 size)
    {
        m_fileSize = size;
    }
    void setStartTime(const QDateTime& t)
    {
        QMutexLocker l(&m_dataMutex);
        m_startTime = t;
    }
    void setEndTime(const QDateTime& t)
    {
        QMutexLocker l(&m_dataMutex);
        m_endTime = t;
    }

    static QString converterTypeToString(ConverterType type);
    static QString priorityToString(Priority priority);
    static ConverterType stringToConverterType(const QString& str);

signals:
    void progressChanged(int progress);
    void statusChanged(Status status);
    void finished(bool success, const QString& message);

private:
    QString m_id;
    QString m_inputFile;
    QString m_outputFile;
    QVariantMap m_params;
    QAtomicInteger<int> m_status;
    QAtomicInt m_progress;
    // Guards the non-atomic members touched from both worker and GUI threads
    // (m_errorMessage, m_startTime, m_endTime).
    mutable QMutex m_dataMutex;
    QString m_errorMessage;
    ConverterType m_converterType;
    Priority m_priority;
    QDateTime m_startTime;
    QDateTime m_endTime;
    QAtomicInt m_cancelled;
    qint64 m_fileSize;
};
#endif // CONVERSION_TASK_H
