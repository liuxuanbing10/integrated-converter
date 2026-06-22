#ifndef CONVERSION_TASK_H
#define CONVERSION_TASK_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QUuid>
#include <QDateTime>
#include <QAtomicInt>
#include <QAtomicInteger>

class ConversionTask : public QObject {
    Q_OBJECT

public:
    enum class Status {
        Pending,
        Running,
        Completed,
        Failed,
        Cancelled
    };

    enum class ConverterType {
        Unknown,
        FFmpeg,
        Pandoc,
        ImageMagick
    };

    enum class Priority {
        Low = 0,
        Normal = 1,
        High = 2
    };

    explicit ConversionTask(QObject* parent = nullptr);
    ConversionTask(const QString& inputFile, const QString& outputFile,
                   const QVariantMap& params, QObject* parent = nullptr);
    ~ConversionTask() override = default;

    QString id() const { return m_id; }
    QString inputFile() const { return m_inputFile; }
    QString outputFile() const { return m_outputFile; }
    QVariantMap params() const { return m_params; }
    Status status() const { return static_cast<Status>(m_status.loadRelaxed()); }
    int progress() const { return m_progress.loadRelaxed(); }
    QString errorMessage() const { return m_errorMessage; }
    ConverterType converterType() const { return m_converterType; }
    Priority priority() const { return m_priority; }
    QDateTime startTime() const { return m_startTime; }
    QDateTime endTime() const { return m_endTime; }
    qint64 durationMs() const;
    bool isCancelled() const { return m_cancelled.loadRelaxed() != 0; }
    qint64 fileSize() const { return m_fileSize; }

    void setInputFile(const QString& file) { m_inputFile = file; }
    void setOutputFile(const QString& file) { m_outputFile = file; }
    void setParams(const QVariantMap& params) { m_params = params; }
    void setConverterType(ConverterType type) { m_converterType = type; }
    void setPriority(Priority priority) { m_priority = priority; }
    void setStatus(Status status);
    void setProgress(int progress);
    void setErrorMessage(const QString& message) { m_errorMessage = message; }
    void requestCancel() { m_cancelled.storeRelease(1); }
    void resetCancelFlag() { m_cancelled.storeRelaxed(0); }
    void setFileSize(qint64 size) { m_fileSize = size; }

    static QString statusToString(Status status);
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
    QString m_errorMessage;
    ConverterType m_converterType;
    Priority m_priority;
    QDateTime m_startTime;
    QDateTime m_endTime;
    QAtomicInt m_cancelled;
    qint64 m_fileSize;
};
#endif // CONVERSION_TASK_H
