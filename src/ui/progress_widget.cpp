#include "progress_widget.h"
#include <QDateTime>
ProgressWidget::ProgressWidget(QWidget* parent)
    : QWidget(parent)
    , m_progressBar(nullptr)
    , m_progressLabel(nullptr)
    , m_currentFileLabel(nullptr)
    , m_totalLabel(nullptr)
    , m_runningLabel(nullptr)
    , m_pendingLabel(nullptr)
    , m_completedLabel(nullptr)
    , m_failedLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_fileSizeLabel(nullptr)
    , m_speedLabel(nullptr)
    , m_bitrateLabel(nullptr)
    , m_totalTasks(0)
    , m_completedTasks(0)
    , m_failedTasks(0)
    , m_runningTasks(0)
    , m_pendingTasks(0)
    , m_overallProgress(0)
    , m_startTime(0)
    , m_fileSize(0)
    , m_processingSpeed(0.0)
    , m_estimatedRemainingMs(0)
    , m_currentBitrate(0.0)
{
    setupUI();
}
ProgressWidget::~ProgressWidget() {
}
void ProgressWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    m_currentFileLabel = new QLabel(tr("当前: 无"));
    m_currentFileLabel->setStyleSheet(
        "padding: 4px 8px; background-color: #fdf3de; border-radius: 8px; color: #bd7e00; font-size: 12px;"
    );
    m_currentFileLabel->setWordWrap(true);
    mainLayout->addWidget(m_currentFileLabel);
    QHBoxLayout* infoLayout = new QHBoxLayout();
    m_fileSizeLabel = new QLabel(tr("大小: --"), this);
    m_speedLabel = new QLabel(tr("速度: --"), this);
    m_bitrateLabel = new QLabel(tr("码率: --"), this);
    QString infoStyle = "padding: 2px 6px; border-radius: 8px; font-size: 11px;";
    m_fileSizeLabel->setStyleSheet(infoStyle + "background-color: #f3f7ff; color: #1664ff;");
    m_speedLabel->setStyleSheet(infoStyle + "background-color: #e2f5eb; color: #2a814b;");
    m_bitrateLabel->setStyleSheet(infoStyle + "background-color: #feeced; color: #d7312a;");
    infoLayout->addWidget(m_fileSizeLabel);
    infoLayout->addWidget(m_speedLabel);
    infoLayout->addWidget(m_bitrateLabel);
    infoLayout->addStretch();
    mainLayout->addLayout(infoLayout);
    QHBoxLayout* progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat(tr("%p%"));
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #dde2e9;"
        "  border-radius: 8px;"
        "  text-align: center;"
        "  background-color: #f7f9fb;"
        "  color: #4e5969;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #1664ff;"
        "  border-radius: 7px;"
        "}"
    );
    progressLayout->addWidget(m_progressBar);
    m_progressLabel = new QLabel("0%", this);
    m_progressLabel->setMinimumWidth(50);
    m_progressLabel->setAlignment(Qt::AlignCenter);
    progressLayout->addWidget(m_progressLabel);
    mainLayout->addLayout(progressLayout);
    QHBoxLayout* statsLayout = new QHBoxLayout();
    m_totalLabel = new QLabel(tr("总计: 0"), this);
    m_runningLabel = new QLabel(tr("运行: 0"), this);
    m_pendingLabel = new QLabel(tr("等待: 0"), this);
    m_completedLabel = new QLabel(tr("成功: 0"), this);
    m_failedLabel = new QLabel(tr("失败: 0"), this);
    m_timeLabel = new QLabel(tr("剩余: --"), this);
    QString labelStyle = "padding: 2px 8px; border-radius: 8px; font-size: 12px;";
    m_totalLabel->setStyleSheet(labelStyle + "background-color: #f3f7ff; color: #1664ff;");
    m_runningLabel->setStyleSheet(labelStyle + "background-color: #fdf3de; color: #bd7e00;");
    m_pendingLabel->setStyleSheet(labelStyle + "background-color: #f3f7ff; color: #387bff;");
    m_completedLabel->setStyleSheet(labelStyle + "background-color: #e2f5eb; color: #2a814b;");
    m_failedLabel->setStyleSheet(labelStyle + "background-color: #feeced; color: #d7312a;");
    m_timeLabel->setStyleSheet(labelStyle + "background-color: #f7f9fb; color: #4e5969;");
    statsLayout->addWidget(m_totalLabel);
    statsLayout->addWidget(m_runningLabel);
    statsLayout->addWidget(m_pendingLabel);
    statsLayout->addWidget(m_completedLabel);
    statsLayout->addWidget(m_failedLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(m_timeLabel);
    mainLayout->addLayout(statsLayout);
}
void ProgressWidget::setTotalTasks(int count) {
    m_totalTasks = count;
    updateDisplay();
}
void ProgressWidget::setCompletedTasks(int count) {
    m_completedTasks = count;
    updateDisplay();
}
void ProgressWidget::setFailedTasks(int count) {
    m_failedTasks = count;
    updateDisplay();
}
void ProgressWidget::setRunningTasks(int count) {
    m_runningTasks = count;
    updateDisplay();
}
void ProgressWidget::setPendingTasks(int count) {
    m_pendingTasks = count;
    updateDisplay();
}
void ProgressWidget::setOverallProgress(int progress) {
    m_overallProgress = qBound(0, progress, 100);
    m_progressBar->setValue(m_overallProgress);
    m_progressLabel->setText(QString("%1%").arg(m_overallProgress));
    emit progressChanged(m_overallProgress);
}
void ProgressWidget::setCurrentFile(const QString& fileName) {
    if (fileName.isEmpty()) {
        m_currentFileLabel->setText(tr("当前: 无"));
    } else {
        QString display = fileName;
        if (display.length() > 50) {
            display = display.left(25) + "..." + display.right(22);
        }
        m_currentFileLabel->setText(tr("当前: %1").arg(display));
    }
}
void ProgressWidget::setFileSize(qint64 size) {
    m_fileSize = size;
    if (size > 0) {
        m_fileSizeLabel->setText(tr("大小: %1").arg(formatFileSize(size)));
    } else {
        m_fileSizeLabel->setText(tr("大小: --"));
    }
}
void ProgressWidget::setProcessingSpeed(double bytesPerSecond) {
    m_processingSpeed = bytesPerSecond;
    if (bytesPerSecond > 0) {
        m_speedLabel->setText(tr("速度: %1/s").arg(formatSpeed(bytesPerSecond)));
    } else {
        m_speedLabel->setText(tr("速度: --"));
    }
}
void ProgressWidget::setEstimatedRemainingTime(qint64 ms) {
    m_estimatedRemainingMs = ms;
    if (ms > 0) {
        int seconds = static_cast<int>(ms / 1000);
        m_timeLabel->setText(tr("剩余: %1").arg(formatTime(seconds)));
    } else {
        m_timeLabel->setText(tr("剩余: --"));
    }
}
void ProgressWidget::setCurrentBitrate(double bitrate) {
    m_currentBitrate = bitrate;
    if (bitrate > 0) {
        m_bitrateLabel->setText(tr("码率: %1 kbps").arg(static_cast<int>(bitrate)));
    } else {
        m_bitrateLabel->setText(tr("码率: --"));
    }
}
void ProgressWidget::setDetailedProgress(int progress, double speed, qint64 remainingMs, double bitrate) {
    setOverallProgress(progress);
    setProcessingSpeed(speed);
    setEstimatedRemainingTime(remainingMs);
    setCurrentBitrate(bitrate);
}
void ProgressWidget::updateFromTaskManager(int total, int pending, int running, int completed, int failed) {
    m_totalTasks = total;
    m_pendingTasks = pending;
    m_runningTasks = running;
    m_completedTasks = completed;
    m_failedTasks = failed;
    if (total > 0) {
        int progress = (completed + failed) * 100 / total;
        setOverallProgress(progress);
    } else {
        setOverallProgress(0);
    }
    updateDisplay();
}
void ProgressWidget::updateDisplay() {
    m_totalLabel->setText(tr("总计: %1").arg(m_totalTasks));
    m_runningLabel->setText(tr("运行: %1").arg(m_runningTasks));
    m_pendingLabel->setText(tr("等待: %1").arg(m_pendingTasks));
    m_completedLabel->setText(tr("成功: %1").arg(m_completedTasks));
    m_failedLabel->setText(tr("失败: %1").arg(m_failedTasks));
    if (m_runningTasks > 0 && m_startTime == 0) {
        m_startTime = QDateTime::currentMSecsSinceEpoch();
    }
    if (m_runningTasks == 0 && m_pendingTasks == 0) {
        m_startTime = 0;
        m_timeLabel->setText(tr("剩余: --"));
        return;
    }
    if (m_startTime > 0 && (m_completedTasks + m_failedTasks) > 0) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;
        double avgTime = elapsed / (double)(m_completedTasks + m_failedTasks);
        int remainingTasks = m_pendingTasks + m_runningTasks;
        int estimatedMs = static_cast<int>(avgTime * remainingTasks);
        int estimatedSec = estimatedMs / 1000;
        m_timeLabel->setText(tr("剩余: %1").arg(formatTime(estimatedSec)));
    }
}
QString ProgressWidget::formatTime(int seconds) const {
    if (seconds <= 0) {
        return "--";
    } else if (seconds < 60) {
        return QString("%1秒").arg(seconds);
    } else if (seconds < 3600) {
        int min = seconds / 60;
        int sec = seconds % 60;
        return QString("%1分%2秒").arg(min).arg(sec);
    } else {
        int hour = seconds / 3600;
        int min = (seconds % 3600) / 60;
        return QString("%1时%2分").arg(hour).arg(min);
    }
}
QString ProgressWidget::formatFileSize(qint64 bytes) const {
    if (bytes < 0) {
        return "--";
    }
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024LL * 1024LL * 1024LL) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}
QString ProgressWidget::formatSpeed(double bytesPerSecond) const {
    if (bytesPerSecond <= 0) {
        return "--";
    }
    if (bytesPerSecond < 1024) {
        return QString("%1 B").arg(static_cast<int>(bytesPerSecond));
    } else if (bytesPerSecond < 1024 * 1024) {
        return QString("%1 KB").arg(bytesPerSecond / 1024.0, 0, 'f', 1);
    } else if (bytesPerSecond < 1024.0 * 1024.0 * 1024.0) {
        return QString("%1 MB").arg(bytesPerSecond / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytesPerSecond / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}
void ProgressWidget::reset() {
    m_totalTasks = 0;
    m_completedTasks = 0;
    m_failedTasks = 0;
    m_runningTasks = 0;
    m_pendingTasks = 0;
    m_overallProgress = 0;
    m_startTime = 0;
    m_fileSize = 0;
    m_processingSpeed = 0.0;
    m_estimatedRemainingMs = 0;
    m_currentBitrate = 0.0;
    m_progressBar->setValue(0);
    m_progressLabel->setText("0%");
    m_currentFileLabel->setText(tr("当前: 无"));
    m_fileSizeLabel->setText(tr("大小: --"));
    m_speedLabel->setText(tr("速度: --"));
    m_bitrateLabel->setText(tr("码率: --"));
    updateDisplay();
}
