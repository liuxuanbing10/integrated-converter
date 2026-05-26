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
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QMap>
#include "format_registry.h"

class TaskListWidget;
class ProgressWidget;
class BatchConversionSummary;
class FileCategoryWidget;
class ConversionParamsDialog;
struct ConversionResult;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onAddFiles();
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
    void onShowSummary();
    void onRetryFailed(const QList<QString>& inputPaths);
    void toggleTheme();
    void onErrorOccurred(const struct ErrorInfo& error);
    void onRetryTriggered(const QString& taskId, int retryCount);
    void updateStatusBar();
    void updateProgressWidget();
    void onTabChanged(int index);
    void onConversionParams();

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

    /// Opens a file dialog that accepts ALL supported formats,
    /// then auto-routes each file to the correct category tab.
    void addFilesAndAutoRoute(const QStringList& filePaths);

    /// Populate the format combo for the given category and restore its saved selection.
    void populateFormatCombo(FormatRegistry::Category cat);

    bool m_darkMode;

    // Category tabs
    QTabWidget* m_tabWidget;
    FileCategoryWidget* m_imageTab;
    FileCategoryWidget* m_docTab;
    FileCategoryWidget* m_audioTab;
    FileCategoryWidget* m_videoTab;

    // External config panel (right of tabs)
    QFrame* m_configPanel;
    QComboBox* m_formatCombo;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_paramsBtn;
    QPushButton* m_convertBtn;

    // Per-category format selection tracking
    QMap<FormatRegistry::Category, QVariant> m_savedFormats;
    // Track last active tab category (replaces static local in onTabChanged)
    FormatRegistry::Category m_lastActiveCategory;
    // Per-category conversion parameters
    QMap<FormatRegistry::Category, QVariantMap> m_conversionParams;

    TaskListWidget* m_taskListWidget;
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
