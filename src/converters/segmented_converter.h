#ifndef SEGMENTED_CONVERTER_H
#define SEGMENTED_CONVERTER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>
#include <QTemporaryFile>

class SegmentedConverter : public QObject {
    Q_OBJECT

public:
    explicit SegmentedConverter(QObject* parent = nullptr);
    ~SegmentedConverter() override;
    void setFFmpegPath(const QString& path) { m_ffmpegPath = path; }
    void setFFprobePath(const QString& path) { m_ffprobePath = path; }
    void setSegmentSize(qint64 bytes);
    void setSegmentCount(int count);
    void setMaxSegments(int max);
    bool convert(const QString& input, const QString& output, const QVariantMap& params);
    void cancel();
    bool isRunning() const { return m_isRunning; }
    int currentSegment() const { return m_currentSegment; }
    int totalSegments() const { return m_totalSegments; }
    int overallProgress() const;

signals:
    void segmentProgress(int segment, int totalSegments, int percent);
    void progressChanged(int overallProgress);
    void statusChanged(const QString& status);
    void conversionFinished(bool success, const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onSegmentFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSegmentReadyRead();
    void onMergeFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool getDuration(const QString& filePath, double& duration);
    bool startNextSegment();
    bool mergeSegments();
    void cleanupTempFiles();
    QString formatTime(double seconds) const;

    QString m_ffmpegPath;
    QString m_ffprobePath;
    QString m_inputFile;
    QString m_outputFile;
    QVariantMap m_params;
    QProcess* m_process;
    QTimer* m_timeoutTimer;
    bool m_isRunning;
    bool m_cancelled;
    double m_totalDuration;
    int m_totalSegments;
    int m_currentSegment;
    double m_segmentDuration;
    QStringList m_segmentFiles;
    QString m_tempDir;
    int m_currentSegmentProgress;
};
#endif // SEGMENTED_CONVERTER_H
