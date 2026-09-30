#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "conversion_coordinator.h"
#include "format_registry.h"

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMenuBar>
#include <QPixmap>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVariant>

class TaskListWidget;
class ProgressWidget;
class PresetChipsWidget;
class PresetLibrary;
namespace Theme
{
enum class Mode;
}
class BatchConversionSummary;
class FileCategoryWidget;
class ConversionParamsDialog;
struct ConversionResult;
struct Preset;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onAddFiles();
    void onStartConversion();
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
    /// §零.2: the results ledger lives in ConversionCoordinator; this slot
    /// only mirrors ledger state into actions/labels when it changes.
    void onResultsChanged();
    void setThemeMode(Theme::Mode mode);
    void togglePauseQueue();
    void onQueuePauseStateChanged(bool paused);
    void onSidebarNav(int index);
    void onSidebarFilterChanged(const QString& text);
    void onErrorOccurred(const struct ErrorInfo& error);
    void onRetryTriggered(const QString& taskId, int retryCount);
    void updateStatusBar();
    void updateProgressWidget();
    void onConversionParams();
    /// §一.1: chips row clicked -> apply recipe to the active category.
    void onApplyPreset(const Preset& preset);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();
    /// §零.2: builds per-category CategoryInput from the widgets and hands
    /// the batch to ConversionCoordinator::submit. When onlyPaths is
    /// non-empty, submit ONLY the listed input paths (retry-failed flow).
    void submitConversionTasks(const QSet<QString>& onlyPaths = QSet<QString>());
    /// View-only: render the coordinator's ledger through BatchConversionSummary.
    void showConversionSummary();

    /// Opens a file dialog that accepts ALL supported formats,
    /// then auto-routes each file to the correct category tab.
    void addFilesAndAutoRoute(const QStringList& filePaths);

private:
    void setNavSelected(int index);

protected:
    // Cancel running tasks before the window is destroyed so child processes
    // (FFmpeg, Pandoc, ImageMagick) are killed instead of becoming orphans.
    void closeEvent(QCloseEvent* event) override;

    // Drag-and-drop support: accept files dropped from Explorer / Finder etc.
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    /// Populate the format combo for the given category and restore its saved selection.
    void populateFormatCombo(FormatRegistry::Category cat);

    // Sidebar navigation + page stack (3FUI-style functional navigation)
    QFrame* m_sidebar = nullptr;
    QLineEdit* m_sidebarSearch = nullptr;
    QList<QPushButton*> m_navButtons;
    QStackedWidget* m_pageStack = nullptr;
    QWidget* m_queuePage = nullptr; // index 0
    FileCategoryWidget* m_imageTab = nullptr;
    FileCategoryWidget* m_docTab = nullptr;
    FileCategoryWidget* m_audioTab = nullptr;
    FileCategoryWidget* m_videoTab = nullptr;
    QWidget* m_settingsPage = nullptr;
    int m_lastNavIndex = 0;

    // External config panel (right of pages)
    QFrame* m_configPanel = nullptr;
    QComboBox* m_formatCombo = nullptr;
    PresetChipsWidget* m_presetChips = nullptr;
    PresetLibrary* m_presetLibrary = nullptr;
    QLineEdit* m_outputDirEdit = nullptr;
    QPushButton* m_paramsBtn = nullptr;
    QPushButton* m_convertBtn = nullptr;

    // Queue-toolbar semantic action buttons (3FUI color semantics)
    QPushButton* m_queuePauseBtn = nullptr;
    QPushButton* m_queueCancelBtn = nullptr;
    QPushButton* m_queueRemoveBtn = nullptr;
    QPushButton* m_queueRetryBtn = nullptr;
    QPushButton* m_queueOpenBtn = nullptr;
    QLabel* m_queueHint = nullptr;

    // Per-category format selection tracking
    QMap<FormatRegistry::Category, QVariant> m_savedFormats;
    // Track last active category (drives the format combo)
    FormatRegistry::Category m_lastActiveCategory;
    // Per-category conversion parameters
    QMap<FormatRegistry::Category, QVariantMap> m_conversionParams;

    // §零.2: session orchestration (submit/route/ledger/pause) lives here,
    // not in this window — MainWindow is assembly + rendering only.
    ConversionCoordinator m_coordinator;

    TaskListWidget* m_taskListWidget;
    ProgressWidget* m_progressWidget;
    QLabel* m_statusLabel;
    QLabel* m_taskStatsLabel;
    QAction* m_startAction;
    QAction* m_cancelAction;
    QAction* m_summaryAction;
    QAction* m_toolbarStartAction;
};

#endif // MAIN_WINDOW_H
