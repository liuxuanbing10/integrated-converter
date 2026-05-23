#include "main_window.h"
#include "task_list_widget.h"
#include "config_panel.h"
#include "file_list_widget.h"
#include "progress_widget.h"
#include "batch_conversion_summary.h"
#include "task_manager.h"
#include "config_manager.h"
#include "logger.h"
#include "error_types.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_fileListWidget(nullptr)
    , m_taskListWidget(nullptr)
    , m_configPanel(nullptr)
    , m_progressWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_statusLabel(nullptr)
    , m_taskStatsLabel(nullptr)
    , m_startAction(nullptr)
    , m_pauseAction(nullptr)
    , m_resumeAction(nullptr)
    , m_cancelAction(nullptr)
    , m_summaryAction(nullptr)
    , m_isPaused(false)
{
    setWindowTitle(tr("集成格式转换工具 v1.0"));
    resize(1200, 800);
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();
    applyStyleSheet();
    LOG_INFO("MainWindow", "主窗口初始化完成");
}

MainWindow::~MainWindow() {
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    QMenu* fileMenu = menuBar->addMenu(tr("文件(&F)"));
    QAction* addFilesAction = fileMenu->addAction(tr("添加文件(&A)"));
    addFilesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);
    QAction* addFolderAction = fileMenu->addAction(tr("添加文件夹(&D)"));
    addFolderAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    connect(addFolderAction, &QAction::triggered, this, &MainWindow::onAddFolder);
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_F4));
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    QMenu* toolMenu = menuBar->addMenu(tr("工具(&T)"));
    m_startAction = toolMenu->addAction(tr("开始转换(&S)"));
    m_startAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(m_startAction, &QAction::triggered, this, &MainWindow::onStartConversion);
    m_pauseAction = toolMenu->addAction(tr("暂停(&P)"));
    m_pauseAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    m_pauseAction->setEnabled(false);
    connect(m_pauseAction, &QAction::triggered, this, &MainWindow::onPauseConversion);
    m_resumeAction = toolMenu->addAction(tr("继续(&R)"));
    m_resumeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_resumeAction->setEnabled(false);
    connect(m_resumeAction, &QAction::triggered, this, &MainWindow::onResumeConversion);
    toolMenu->addSeparator();
    m_cancelAction = toolMenu->addAction(tr("取消全部(&C)"));
    m_cancelAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    connect(m_cancelAction, &QAction::triggered, this, &MainWindow::onCancelAll);
    toolMenu->addSeparator();
    m_summaryAction = toolMenu->addAction(tr("查看汇总(&V)"));
    m_summaryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_summaryAction->setEnabled(false);
    connect(m_summaryAction, &QAction::triggered, this, &MainWindow::onShowSummary);

    QMenu* helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    QAction* aboutAction = helpMenu->addAction(tr("关于(&A)"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar(tr("主工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));

    QAction* addFilesAction = toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), tr("添加文件"));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);

    QAction* addFolderAction = toolBar->addAction(style()->standardIcon(QStyle::SP_DirIcon), tr("添加文件夹"));
    connect(addFolderAction, &QAction::triggered, this, &MainWindow::onAddFolder);

    toolBar->addSeparator();

    m_toolbarStartAction = toolBar->addAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("开始转换"));
    connect(m_toolbarStartAction, &QAction::triggered, this, &MainWindow::onStartConversion);
}

void MainWindow::setupStatusBar() {
    QStatusBar* statusBar = this->statusBar();
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("padding: 2px 8px;");
    m_taskStatsLabel = new QLabel();
    m_taskStatsLabel->setStyleSheet("padding: 2px 8px;");
    statusBar->addWidget(m_statusLabel, 1);
    statusBar->addPermanentWidget(m_taskStatsLabel);
}

void MainWindow::setupCentralWidget() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    QWidget* topPanel = new QWidget();
    QHBoxLayout* topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    QGroupBox* fileListGroup = new QGroupBox(tr("文件列表"));
    fileListGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* fileListLayout = new QVBoxLayout(fileListGroup);
    m_fileListWidget = new FileListWidget();
    fileListLayout->addWidget(m_fileListWidget);
    topLayout->addWidget(fileListGroup, 2);

    QGroupBox* configGroup = new QGroupBox(tr("转换设置"));
    configGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 2px solid #2196F3; border-radius: 6px; margin-top: 8px; padding-top: 8px; background-color: #f8fbff; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 8px; color: #2196F3; font-size: 13px; }"
    );
    QVBoxLayout* configLayout = new QVBoxLayout(configGroup);
    m_configPanel = new ConfigPanel();
    configLayout->addWidget(m_configPanel);
    topLayout->addWidget(configGroup, 1);

    mainLayout->addWidget(topPanel, 2);

    QGroupBox* taskListGroup = new QGroupBox(tr("任务列表"));
    taskListGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* taskListLayout = new QVBoxLayout(taskListGroup);
    m_progressWidget = new ProgressWidget();
    taskListLayout->addWidget(m_progressWidget);
    m_taskListWidget = new TaskListWidget();
    taskListLayout->addWidget(m_taskListWidget, 1);
    mainLayout->addWidget(taskListGroup, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupConnections() {
    TaskManager* tm = TaskManager::instance();
    connect(tm, &TaskManager::taskAdded, this, &MainWindow::onTaskAdded);
    connect(tm, &TaskManager::taskStarted, this, &MainWindow::onTaskStarted);
    connect(tm, &TaskManager::taskProgressChanged, this, &MainWindow::onTaskProgressChanged);
    connect(tm, &TaskManager::taskCompleted, this, &MainWindow::onTaskCompleted);
    connect(tm, &TaskManager::allTasksCompleted, this, &MainWindow::onAllTasksCompleted);
    connect(m_fileListWidget, &FileListWidget::filesAdded, this, &MainWindow::onFilesAdded);
    connect(m_fileListWidget, &FileListWidget::fileCountChanged, this, &MainWindow::onFileCountChanged);
}

void MainWindow::applyStyleSheet() {
    QString styleSheet = R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        QMenuBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
        }
        QMenuBar::item {
            padding: 6px 12px;
            background: transparent;
        }
        QMenuBar::item:selected {
            background-color: #e3f2fd;
        }
        QMenu {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
        }
        QMenu::item {
            padding: 6px 30px 6px 20px;
        }
        QMenu::item:selected {
            background-color: #e3f2fd;
        }
        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
            spacing: 5px;
            padding: 4px;
        }
        QToolBar QToolButton {
            padding: 6px;
            border-radius: 4px;
            border: 1px solid transparent;
        }
        QToolBar QToolButton:hover {
            background-color: #e3f2fd;
        }
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #e0e0e0;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #ccc;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    )";
    setStyleSheet(styleSheet);
}

void MainWindow::onAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, tr("选择文件"),
        QString(),
        tr("所有支持格式 (*.mp4 *.avi *.mkv *.mp3 *.wav *.md *.txt *.docx);;"
           "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv);;"
           "音频文件 (*.mp3 *.wav *.flac *.aac);;"
           "文档文件 (*.md *.txt *.docx *.pdf);;"
           "所有文件 (*)")
    );
    if (!files.isEmpty()) {
        m_fileListWidget->addFiles(files);
    }
}

void MainWindow::onAddFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, tr("选择文件夹"));
    if (!folder.isEmpty()) {
        m_fileListWidget->addFolder(folder, true);
    }
}

void MainWindow::onStartConversion() {
    if (m_fileListWidget->isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先添加要转换的文件"));
        return;
    }
    m_conversionResults.clear();
    submitConversionTasks();
    TaskManager::instance()->start();
    m_statusLabel->setText(tr("正在转换..."));
    m_startAction->setEnabled(false);
    m_pauseAction->setEnabled(true);
    m_resumeAction->setEnabled(false);
    m_summaryAction->setEnabled(false);
    m_toolbarStartAction->setEnabled(false);
    LOG_INFO("MainWindow", "开始转换任务");
}

void MainWindow::onPauseConversion() {
    TaskManager::instance()->pause();
    m_isPaused = true;
    m_statusLabel->setText(tr("已暂停"));
    m_pauseAction->setEnabled(false);
    m_resumeAction->setEnabled(true);
    LOG_INFO("MainWindow", "暂停转换");
}

void MainWindow::onResumeConversion() {
    TaskManager::instance()->resume();
    m_isPaused = false;
    m_statusLabel->setText(tr("正在转换..."));
    m_pauseAction->setEnabled(true);
    m_resumeAction->setEnabled(false);
    LOG_INFO("MainWindow", "继续转换");
}

void MainWindow::onCancelAll() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("确认"),
        tr("确定要取消所有任务吗？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        TaskManager::instance()->cancelAllTasks();
        m_statusLabel->setText(tr("已取消"));
        m_startAction->setEnabled(true);
        m_pauseAction->setEnabled(false);
        m_resumeAction->setEnabled(false);
        m_toolbarStartAction->setEnabled(true);
        m_isPaused = false;
        LOG_INFO("MainWindow", "取消所有任务");
    }
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("关于"),
        tr("<h3>集成格式转换工具 v1.0.0</h3>"
           "<p>基于FFmpeg和Pandoc的多功能文件转换工具</p>"
           "<p>支持功能：</p>"
           "<ul>"
           "<li>视频格式转换（MP4, AVI, MKV等）</li>"
           "<li>音频格式转换（MP3, WAV, FLAC等）</li>"
           "<li>文档格式转换（Markdown, DOCX, PDF等）</li>"
           "</ul>"
           "<p> 2024 ConverterTools</p>"));
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onTaskAdded(const QString& taskId) {
    Q_UNUSED(taskId);
    updateStatusBar();
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
}

void MainWindow::onTaskStarted(const QString& taskId) {
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task) {
        m_currentConvertingFile = task->inputFile();
        QFileInfo fi(m_currentConvertingFile);
        m_progressWidget->setCurrentFile(fi.fileName());
    }
    updateStatusBar();
    updateProgressWidget();
}

void MainWindow::onTaskProgressChanged(const QString& taskId, int progress) {
    Q_UNUSED(taskId);
    Q_UNUSED(progress);
    updateProgressWidget();
}

void MainWindow::onTaskCompleted(const QString& taskId, bool success) {
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task) {
        ConversionResult result;
        result.inputPath = task->inputFile();
        result.outputPath = task->outputFile();
        result.success = success;
        result.errorMessage = task->errorMessage();
        result.durationMs = task->durationMs();
        m_conversionResults.append(result);
    }
    updateStatusBar();
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
}

void MainWindow::onAllTasksCompleted() {
    m_statusLabel->setText(tr("所有任务已完成"));
    m_startAction->setEnabled(true);
    m_pauseAction->setEnabled(false);
    m_resumeAction->setEnabled(false);
    m_isPaused = false;
    m_summaryAction->setEnabled(!m_conversionResults.isEmpty());
    m_toolbarStartAction->setEnabled(true);
    m_progressWidget->setCurrentFile(QString());
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
    if (ConfigManager::instance().value("showNotification", true).toBool()) {
        int completed = TaskManager::instance()->completedCount();
        int failed = TaskManager::instance()->failedCount();
        QString msg = tr("转换完成！\n成功: %1\n失败: %2").arg(completed).arg(failed);
        if (failed > 0) {
            QMessageBox::warning(this, tr("转换完成"), msg);
        } else {
            QMessageBox::information(this, tr("转换完成"), msg);
        }
    }
    if (!m_conversionResults.isEmpty()) {
        showConversionSummary();
    }
    LOG_INFO("MainWindow", "所有任务已完成");
}

void MainWindow::onFilesAdded(const QList<FileInfo>& files) {
    Q_UNUSED(files);
    m_statusLabel->setText(tr("已添加 %1 个文件").arg(files.size()));
}

void MainWindow::onFileCountChanged(int count) {
    if (count == 0) {
        m_statusLabel->setText(tr("就绪"));
    }
}

void MainWindow::onShowSummary() {
    showConversionSummary();
}

void MainWindow::onRetryFailed(const QList<QString>& inputPaths) {
    if (inputPaths.isEmpty()) return;
    m_fileListWidget->clear();
    m_fileListWidget->addFiles(inputPaths);
    m_conversionResults.clear();
    m_startAction->setEnabled(false);
    onStartConversion();
}

void MainWindow::updateStatusBar() {
    TaskManager* tm = TaskManager::instance();
    int total = tm->totalTaskCount();
    int running = tm->runningCount();
    int pending = tm->pendingCount();
    int completed = tm->completedCount();
    int failed = tm->failedCount();
    m_taskStatsLabel->setText(
        tr("总计: %1 | 运行: %2 | 等待: %3 | 完成: %4 | 失败: %5")
        .arg(total).arg(running).arg(pending).arg(completed).arg(failed)
    );
}

void MainWindow::updateProgressWidget() {
    TaskManager* tm = TaskManager::instance();
    m_progressWidget->updateFromTaskManager(
        tm->totalTaskCount(),
        tm->pendingCount(),
        tm->runningCount(),
        tm->completedCount(),
        tm->failedCount()
    );
}

void MainWindow::submitConversionTasks() {
    QList<FileInfo> files = m_fileListWidget->allFiles();
    if (files.isEmpty()) return;
    QString outputFormat = m_configPanel->selectedOutputFormat();
    QString outputDir = m_configPanel->outputDirectory();
    QVariantMap baseParams = m_configPanel->conversionParams();
    for (const FileInfo& fileInfo : files) {
        QFileInfo fi(fileInfo.filePath);
        QString baseName = fi.completeBaseName();
        QString outputFile = outputDir + "/" + baseName + "." + outputFormat;
        QVariantMap params = baseParams;
        params["outputFormat"] = outputFormat;
        QString ext = fi.suffix().toLower();
        QStringList videoFormats = {"mp4", "avi", "mkv", "mov", "flv", "wmv", "webm"};
        QStringList audioFormats = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma"};
        QStringList docFormats = {"md", "txt", "docx", "pdf", "html", "rtf", "epub", "odt", "tex"};
        if (videoFormats.contains(ext) || audioFormats.contains(ext)) {
            params["converter"] = "FFmpeg";
        } else if (docFormats.contains(ext)) {
            params["converter"] = "Pandoc";
        } else {
            params["converter"] = "FFmpeg";
        }
        TaskManager::instance()->addTask(fileInfo.filePath, outputFile, params);
    }
    LOG_INFO("MainWindow", QString("提交 %1 个转换任务").arg(files.size()));
}

void MainWindow::showConversionSummary() {
    BatchConversionSummary summary(this);
    summary.setResults(m_conversionResults);
    connect(&summary, &BatchConversionSummary::retryRequested,
            this, &MainWindow::onRetryFailed);
    summary.exec();
}

void MainWindow::onErrorOccurred(const ErrorInfo& error) {
    Q_UNUSED(error);
}

void MainWindow::onRetryTriggered(const QString& taskId, int retryCount) {
    Q_UNUSED(taskId);
    Q_UNUSED(retryCount);
}
