#ifndef PROGRESS_WIDGET_H
#define PROGRESS_WIDGET_H
#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
class ProgressWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProgressWidget(QWidget* parent = nullptr);
    ~ProgressWidget() override;
    int totalTasks() const { return m_totalTasks; }
    int completedTasks() const { return m_completedTasks; }
    int failedTasks() const { return m_failedTasks; }
    int runningTasks() const { return m_runningTasks; }
    int pendingTasks() const { return m_pendingTasks; }
    int overallProgress() const { return m_overallProgress; }
signals:
    void progressChanged(int progress);
public slots:
    void setTotalTasks(int count);
    void setCompletedTasks(int count);
    void setFailedTasks(int count);
    void setRunningTasks(int count);
    void setPendingTasks(int count);
    void setOverallProgress(int progress);
    void setCurrentFile(const QString& fileName);
    void updateFromTaskManager(int total, int pending, int running, int completed, int failed);
    void setFileSize(qint64 size);
    void setProcessingSpeed(double bytesPerSecond);
    void setEstimatedRemainingTime(qint64 ms);
    void setCurrentBitrate(double bitrate);
    void setDetailedProgress(int progress, double speed, qint64 remainingMs, double bitrate);
    void reset();
private:
    void setupUI();
    void updateDisplay();
    QString formatTime(int seconds) const;
    QString formatFileSize(qint64 bytes) const;
    QString formatSpeed(double bytesPerSecond) const;
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;
    QLabel* m_currentFileLabel;
    QLabel* m_totalLabel;
    QLabel* m_runningLabel;
    QLabel* m_pendingLabel;
    QLabel* m_completedLabel;
    QLabel* m_failedLabel;
    QLabel* m_timeLabel;
    QLabel* m_fileSizeLabel;
    QLabel* m_speedLabel;
    QLabel* m_bitrateLabel;
    int m_totalTasks;
    int m_completedTasks;
    int m_failedTasks;
    int m_runningTasks;
    int m_pendingTasks;
    int m_overallProgress;
    qint64 m_startTime;
    qint64 m_fileSize;
    double m_processingSpeed;
    qint64 m_estimatedRemainingMs;
    double m_currentBitrate;
};
#endif // PROGRESS_WIDGET_H
