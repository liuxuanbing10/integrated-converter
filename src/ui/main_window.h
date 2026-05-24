#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QLabel>
#include <QAction>
#include <QDateTime>
#include <QPixmap>
#include <QVariant>
class TaskListWidget;
class ConfigPanel;
class FileListWidget;
class ProgressWidget;
class BatchConversionSummary;
struct ConversionResult;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
private slots:
    void onAddFiles();
    void onAddFolder();
    void onStartConversion();
    void onPauseConversion();
    void onResumeConversion();
    void onCancelAll();
    void onAbout();
    void onExit();
    void onTaskAdded(const QString& taskId);
    void onTaskStarted(const QString& taskId);
    void onTaskProgressChanged(const QString& taskId, int progress);
    void onTaskCompleted(const QString& taskId, bool success);
    void onAllTasksCompleted();
    void onFilesAdded(const QList<struct FileInfo>& files);
    void onFileCountChanged(int count);
    void onShowSummary();
    void onRetryFailed(const QList<QString>& inputPaths);
    void toggleTheme();
    void onErrorOccurred(const struct ErrorInfo& error);
    void onRetryTriggered(const QString& taskId, int retryCount);
    void updateStatusBar();
    void updateProgressWidget();
private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();
    void submitConversionTasks();
    void showConversionSummary();
    void showErrorDialog(const struct ErrorInfo& error);
    void updateErrorIcon();
    void applyLightTheme();
    void applyDarkTheme();
    bool m_darkMode;
    FileListWidget* m_fileListWidget;
    TaskListWidget* m_taskListWidget;
    ConfigPanel* m_configPanel;
    ProgressWidget* m_progressWidget;
    QSplitter* m_mainSplitter;
    QLabel* m_statusLabel;
    QLabel* m_taskStatsLabel;
    QLabel* m_errorIconLabel;
    QAction* m_startAction;
    QAction* m_pauseAction;
    QAction* m_resumeAction;
    QAction* m_cancelAction;
    QAction* m_summaryAction;
    QAction* m_toolbarStartAction;
    QAction* m_toolbarPauseAction;
    QAction* m_toolbarCancelAction;
    QAction* m_toolbarSummaryAction;
    bool m_isPaused;
    int m_errorCount;
    QList<ConversionResult> m_conversionResults;
    QString m_currentConvertingFile;
};
#endif // MAIN_WINDOW_H
